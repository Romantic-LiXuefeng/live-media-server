#include "rtmp_handshake.hpp"
#include "kernel_log.hpp"
#include "kernel_errno.hpp"

#include <time.h>
#include <string.h>

using namespace _srs_internal;

// for openssl_HMACsha256
#include <openssl/evp.h>
#include <openssl/hmac.h>
// for __openssl_generate_key
#include <openssl/dh.h>

void random_generate(char* bytes, int size)
{
    static bool _random_initialized = false;
    if (!_random_initialized) {
        srand(0);
        _random_initialized = true;
        log_trace("srand initialized the random.");
    }

    for (int i = 0; i < size; i++) {
        // the common value in [0x0f, 0xf0]
        bytes[i] = 0x0f + (rand() % (256 - 0x0f - 0x0f));
    }
}

bool bytes_equals(void* pa, void* pb, int size)
{
    u_int8_t* a = (u_int8_t*)pa;
    u_int8_t* b = (u_int8_t*)pb;

    if (!a && !b) {
        return true;
    }

    if (!a || !b) {
        return false;
    }

    for(int i = 0; i < size; i++){
        if(a[i] != b[i]){
            return false;
        }
    }

    return true;
}

#if OPENSSL_VERSION_NUMBER < 0x10100000L

static HMAC_CTX *HMAC_CTX_new(void)
{
    HMAC_CTX *ctx = (HMAC_CTX *)malloc(sizeof(*ctx));
    if (ctx != NULL) {
        HMAC_CTX_init(ctx);
    }
    return ctx;
}

static void HMAC_CTX_free(HMAC_CTX *ctx)
{
    if (ctx != NULL) {
        HMAC_CTX_cleanup(ctx);
        free(ctx);
    }
}

static void DH_get0_key(const DH *dh, const BIGNUM **pub_key, const BIGNUM **priv_key)
{
    if (pub_key != NULL) {
        *pub_key = dh->pub_key;
    }
    if (priv_key != NULL) {
        *priv_key = dh->priv_key;
    }
}

static int DH_set0_pqg(DH *dh, BIGNUM *p, BIGNUM *q, BIGNUM *g)
{
    /* If the fields p and g in d are NULL, the corresponding input
     * parameters MUST be non-NULL.  q may remain NULL.
     */
    if ((dh->p == NULL && p == NULL)
        || (dh->g == NULL && g == NULL))
        return 0;

    if (p != NULL) {
        BN_free(dh->p);
        dh->p = p;
    }
    if (q != NULL) {
        BN_free(dh->q);
        dh->q = q;
    }
    if (g != NULL) {
        BN_free(dh->g);
        dh->g = g;
    }

    if (q != NULL) {
        dh->length = BN_num_bits(q);
    }

    return 1;
}

static int DH_set_length(DH *dh, long length)
{
    dh->length = length;
    return 1;
}

#endif

namespace _srs_internal
{
    // 68bytes FMS key which is used to sign the sever packet.
    uint8_t SrsGenuineFMSKey[] = {
        0x47, 0x65, 0x6e, 0x75, 0x69, 0x6e, 0x65, 0x20,
        0x41, 0x64, 0x6f, 0x62, 0x65, 0x20, 0x46, 0x6c,
        0x61, 0x73, 0x68, 0x20, 0x4d, 0x65, 0x64, 0x69,
        0x61, 0x20, 0x53, 0x65, 0x72, 0x76, 0x65, 0x72,
        0x20, 0x30, 0x30, 0x31, // Genuine Adobe Flash Media Server 001
        0xf0, 0xee, 0xc2, 0x4a, 0x80, 0x68, 0xbe, 0xe8,
        0x2e, 0x00, 0xd0, 0xd1, 0x02, 0x9e, 0x7e, 0x57,
        0x6e, 0xec, 0x5d, 0x2d, 0x29, 0x80, 0x6f, 0xab,
        0x93, 0xb8, 0xe6, 0x36, 0xcf, 0xeb, 0x31, 0xae
    }; // 68

    // 62bytes FP key which is used to sign the client packet.
    uint8_t SrsGenuineFPKey[] = {
        0x47, 0x65, 0x6E, 0x75, 0x69, 0x6E, 0x65, 0x20,
        0x41, 0x64, 0x6F, 0x62, 0x65, 0x20, 0x46, 0x6C,
        0x61, 0x73, 0x68, 0x20, 0x50, 0x6C, 0x61, 0x79,
        0x65, 0x72, 0x20, 0x30, 0x30, 0x31, // Genuine Adobe Flash Player 001
        0xF0, 0xEE, 0xC2, 0x4A, 0x80, 0x68, 0xBE, 0xE8,
        0x2E, 0x00, 0xD0, 0xD1, 0x02, 0x9E, 0x7E, 0x57,
        0x6E, 0xEC, 0x5D, 0x2D, 0x29, 0x80, 0x6F, 0xAB,
        0x93, 0xB8, 0xE6, 0x36, 0xCF, 0xEB, 0x31, 0xAE
    }; // 62

    int do_openssl_HMACsha256(HMAC_CTX* ctx, const void* data, int data_size, void* digest, unsigned int* digest_size)
    {
        int ret = ERROR_SUCCESS;

        if (HMAC_Update(ctx, (unsigned char *) data, data_size) < 0) {
            ret = ERROR_OpenSslSha256Update;
            return ret;
        }

        if (HMAC_Final(ctx, (unsigned char *) digest, digest_size) < 0) {
            ret = ERROR_OpenSslSha256Final;
            return ret;
        }

        return ret;
    }
    /**
     * sha256 digest algorithm.
     * @param key the sha256 key, NULL to use EVP_Digest, for instance,
     *       hashlib.sha256(data).digest().
     */
    int openssl_HMACsha256(const void* key, int key_size, const void* data, int data_size, void* digest)
    {
        int ret = ERROR_SUCCESS;

        unsigned int digest_size = 0;

        unsigned char* temp_key = (unsigned char*)key;
        unsigned char* temp_digest = (unsigned char*)digest;

        if (key == NULL) {
            // use data to digest.
            // @see ./crypto/sha/sha256t.c
            // @see ./crypto/evp/digest.c
            if (EVP_Digest(data, data_size, temp_digest, &digest_size, EVP_sha256(), NULL) < 0)
            {
                ret = ERROR_OpenSslSha256EvpDigest;
                return ret;
            }
        } else {
            // use key-data to digest.
            HMAC_CTX *ctx = HMAC_CTX_new();
            if (ctx == NULL) {
                ret = ERROR_OpenSslCreateHMAC;
                return ret;
            }
            // @remark, if no key, use EVP_Digest to digest,
            // for instance, in python, hashlib.sha256(data).digest().
            if (HMAC_Init_ex(ctx, temp_key, key_size, EVP_sha256(), NULL) < 0) {
                ret = ERROR_OpenSslSha256Init;
                HMAC_CTX_free(ctx);
                return ret;
            }

            ret = do_openssl_HMACsha256(ctx, data, data_size, temp_digest, &digest_size);
            HMAC_CTX_free(ctx);

            if (ret != ERROR_SUCCESS) {
                return ret;
            }
        }

        if (digest_size != 32) {
            ret = ERROR_OpenSslSha256DigestSize;
            return ret;
        }

        return ret;
    }

#define RFC2409_PRIME_1024 \
"FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD1" \
"29024E088A67CC74020BBEA63B139B22514A08798E3404DD" \
"EF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245" \
"E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7ED" \
"EE386BFB5A899FA5AE9F24117C4B1FE649286651ECE65381" \
"FFFFFFFFFFFFFFFF"

    SrsDH::SrsDH()
    {
        pdh = NULL;
    }

    SrsDH::~SrsDH()
    {
        close();
    }

    void SrsDH::close()
    {
        if (pdh != NULL) {
            DH_free(pdh);
            pdh = NULL;
        }
    }

    int SrsDH::initialize(bool ensure_128bytes_public_key)
    {
        int ret = ERROR_SUCCESS;

        for (;;) {
            if ((ret = do_initialize()) != ERROR_SUCCESS) {
                return ret;
            }

            if (ensure_128bytes_public_key) {
                const BIGNUM *pub_key = NULL;
                DH_get0_key(pdh, &pub_key, NULL);
                int32_t key_size = BN_num_bytes(pub_key);
                if (key_size != 128) {
                    log_warn("regenerate 128B key, current=%dB", key_size);
                    continue;
                }
            }

            break;
        }

        return ret;
    }

    int SrsDH::copy_public_key(char* pkey, int32_t& pkey_size)
    {
        int ret = ERROR_SUCCESS;

        // copy public key to bytes.
        // sometimes, the key_size is 127, seems ok.
        const BIGNUM *pub_key = NULL;
        DH_get0_key(pdh, &pub_key, NULL);
        int32_t key_size = BN_num_bytes(pub_key);
        DAssert(key_size > 0);

        // maybe the key_size is 127, but dh will write all 128bytes pkey,
        // so, donot need to set/initialize the pkey.
        // @see https://github.com/ossrs/srs/issues/165
        key_size = BN_bn2bin(pub_key, (unsigned char*)pkey);
        DAssert(key_size > 0);

        // output the size of public key.
        // @see https://github.com/ossrs/srs/issues/165
        DAssert(key_size <= pkey_size);
        pkey_size = key_size;

        return ret;
    }

    int SrsDH::copy_shared_key(const char* ppkey, int32_t ppkey_size, char* skey, int32_t& skey_size)
    {
        int ret = ERROR_SUCCESS;

        BIGNUM* ppk = NULL;
        if ((ppk = BN_bin2bn((const unsigned char*)ppkey, ppkey_size, 0)) == NULL) {
            ret = ERROR_OpenSslGetPeerPublicKey;
            return ret;
        }

        // if failed, donot return, do cleanup, @see ./test/dhtest.c:168
        // maybe the key_size is 127, but dh will write all 128bytes skey,
        // so, donot need to set/initialize the skey.
        // @see https://github.com/ossrs/srs/issues/165
        int32_t key_size = DH_compute_key((unsigned char*)skey, ppk, pdh);

        if (key_size < ppkey_size) {
            log_warn("shared key size=%d, ppk_size=%d", key_size, ppkey_size);
        }

        if (key_size < 0 || key_size > skey_size) {
            ret = ERROR_OpenSslComputeSharedKey;
        } else {
            skey_size = key_size;
        }

        if (ppk) {
            BN_free(ppk);
        }

        return ret;
    }

    int SrsDH::do_initialize()
    {
        int ret = ERROR_SUCCESS;

        int32_t bits_count = 1024;

        close();

        //1. Create the DH
        if ((pdh = DH_new()) == NULL) {
            ret = ERROR_OpenSslCreateDH;
            return ret;
        }

        //2. Create his internal p and g
        BIGNUM *p, *g;
        if ((p = BN_new()) == NULL) {
            ret = ERROR_OpenSslCreateP;
            return ret;
        }
        if ((g = BN_new()) == NULL) {
            ret = ERROR_OpenSslCreateG;
            BN_free(p);
            return ret;
        }
        DH_set0_pqg(pdh, p, NULL, g);

        //3. initialize p and g, @see ./test/ectest.c:260
        if (!BN_hex2bn(&p, RFC2409_PRIME_1024)) {
            ret = ERROR_OpenSslParseP1024;
            return ret;
        }
        // @see ./test/bntest.c:1764
        if (!BN_set_word(g, 2)) {
            ret = ERROR_OpenSslSetG;
            return ret;
        }

        // 4. Set the key length
        DH_set_length(pdh, bits_count);

        // 5. Generate private and public key
        // @see ./test/dhtest.c:152
        if (!DH_generate_key(pdh)) {
            ret = ERROR_OpenSslGenerateDHKeys;
            return ret;
        }

        return ret;
    }

    key_block::key_block()
    {
        offset = (int32_t)rand();
        random0 = NULL;
        random1 = NULL;

        int valid_offset = calc_valid_offset();
        DAssert(valid_offset >= 0);

        random0_size = valid_offset;
        if (random0_size > 0) {
            random0 = new char[random0_size];
            random_generate(random0, random0_size);
        }

        random_generate(key, sizeof(key));

        random1_size = 764 - valid_offset - 128 - 4;
        if (random1_size > 0) {
            random1 = new char[random1_size];
            random_generate(random1, random1_size);
        }
    }

    key_block::~key_block()
    {
        DFreeArray(random0);
        DFreeArray(random1);
    }

    int key_block::parse(DStream* stream)
    {
        int ret = ERROR_SUCCESS;

        // the key must be 764 bytes.
        DAssert(stream->left() >= 764);

        // read the last offset first, 760-763
        stream->skip(764 - sizeof(int32_t));
        stream->read4Bytes(offset);

        // reset stream to read others.
        stream->skip(-764);

        int valid_offset = calc_valid_offset();
        DAssert(valid_offset >= 0);

        random0_size = valid_offset;
        if (random0_size > 0) {
            DFreeArray(random0);
            random0 = new char[random0_size];
            stream->readBytes(random0, random0_size);
        }

        stream->readBytes(key, 128);

        random1_size = 764 - valid_offset - 128 - 4;
        if (random1_size > 0) {
            DFreeArray(random1);
            random1 = new char[random1_size];
            stream->readBytes(random1, random1_size);
        }

        return ret;
    }

    int key_block::calc_valid_offset()
    {
        int max_offset_size = 764 - 128 - 4;

        int valid_offset = 0;
        uint8_t* pp = (uint8_t*)&offset;
        valid_offset += *pp++;
        valid_offset += *pp++;
        valid_offset += *pp++;
        valid_offset += *pp++;

        return valid_offset % max_offset_size;
    }

    digest_block::digest_block()
    {
        offset = (int32_t)rand();
        random0 = NULL;
        random1 = NULL;

        int valid_offset = calc_valid_offset();
        DAssert(valid_offset >= 0);

        random0_size = valid_offset;
        if (random0_size > 0) {
            random0 = new char[random0_size];
            random_generate(random0, random0_size);
        }

        random_generate(digest, sizeof(digest));

        random1_size = 764 - 4 - valid_offset - 32;
        if (random1_size > 0) {
            random1 = new char[random1_size];
            random_generate(random1, random1_size);
        }
    }

    digest_block::~digest_block()
    {
        DFreeArray(random0);
        DFreeArray(random1);
    }

    int digest_block::parse(DStream* stream)
    {
        int ret = ERROR_SUCCESS;

        // the digest must be 764 bytes.
        DAssert(stream->left() >= 764);

        stream->read4Bytes(offset);

        int valid_offset = calc_valid_offset();
        DAssert(valid_offset >= 0);

        random0_size = valid_offset;
        if (random0_size > 0) {
            DFreeArray(random0);
            random0 = new char[random0_size];
            stream->readBytes(random0, random0_size);
        }

        stream->readBytes(digest, 32);

        random1_size = 764 - 4 - valid_offset - 32;
        if (random1_size > 0) {
            DFreeArray(random1);
            random1 = new char[random1_size];
            stream->readBytes(random1, random1_size);
        }

        return ret;
    }

    int digest_block::calc_valid_offset()
    {
        int max_offset_size = 764 - 32 - 4;

        int valid_offset = 0;
        uint8_t* pp = (uint8_t*)&offset;
        valid_offset += *pp++;
        valid_offset += *pp++;
        valid_offset += *pp++;
        valid_offset += *pp++;

        return valid_offset % max_offset_size;
    }

    c1s1_strategy::c1s1_strategy()
    {
    }

    c1s1_strategy::~c1s1_strategy()
    {
    }

    char* c1s1_strategy::get_digest()
    {
        return digest.digest;
    }

    char* c1s1_strategy::get_key()
    {
        return key.key;
    }

    int c1s1_strategy::dump(c1s1* owner, char* _c1s1, int size)
    {
        DAssert(size == 1536);
        return copy_to(owner, _c1s1, size, true);
    }

    int c1s1_strategy::c1_create(c1s1* owner)
    {
        int ret = ERROR_SUCCESS;

        // generate digest
        char* c1_digest = NULL;

        if ((ret = calc_c1_digest(owner, c1_digest)) != ERROR_SUCCESS) {
            log_error("sign c1 error, failed to calc digest. ret=%d", ret);
            return ret;
        }

        DAssert(c1_digest != NULL);
        DAutoFreeArray(char, c1_digest);

        memcpy(digest.digest, c1_digest, 32);

        return ret;
    }

    int c1s1_strategy::c1_validate_digest(c1s1* owner, bool& is_valid)
    {
        int ret = ERROR_SUCCESS;

        char* c1_digest = NULL;

        if ((ret = calc_c1_digest(owner, c1_digest)) != ERROR_SUCCESS) {
            log_error("validate c1 error, failed to calc digest. ret=%d", ret);
            return ret;
        }

        DAssert(c1_digest != NULL);
        DAutoFreeArray(char, c1_digest);

        is_valid = bytes_equals(digest.digest, c1_digest, 32);

        return ret;
    }

    int c1s1_strategy::s1_create(c1s1* owner, c1s1* c1)
    {
        int ret = ERROR_SUCCESS;

        SrsDH dh;

        // ensure generate 128bytes public key.
        if ((ret = dh.initialize(true)) != ERROR_SUCCESS) {
            return ret;
        }

        // directly generate the public key.
        // @see: https://github.com/ossrs/srs/issues/148
        int pkey_size = 128;
        if ((ret = dh.copy_shared_key(c1->get_key(), 128, key.key, pkey_size)) != ERROR_SUCCESS) {
            log_error("calc s1 key failed. ret=%d", ret);
            return ret;
        }

        // although the public key is always 128bytes, but the share key maybe not.
        // we just ignore the actual key size, but if need to use the key, must use the actual size.
        // TODO: FIXME: use the actual key size.
        //DAssert(pkey_size == 128);
        log_verbose("calc s1 key success.");

        char* s1_digest = NULL;
        if ((ret = calc_s1_digest(owner, s1_digest))  != ERROR_SUCCESS) {
            log_error("calc s1 digest failed. ret=%d", ret);
            return ret;
        }
        log_verbose("calc s1 digest success.");

        DAssert(s1_digest != NULL);
        DAutoFreeArray(char, s1_digest);

        memcpy(digest.digest, s1_digest, 32);
        log_verbose("copy s1 key success.");

        return ret;
    }

    int c1s1_strategy::s1_validate_digest(c1s1* owner, bool& is_valid)
    {
        int ret = ERROR_SUCCESS;

        char* s1_digest = NULL;

        if ((ret = calc_s1_digest(owner, s1_digest)) != ERROR_SUCCESS) {
            log_error("validate s1 error, failed to calc digest. ret=%d", ret);
            return ret;
        }

        DAssert(s1_digest != NULL);
        DAutoFreeArray(char, s1_digest);

        is_valid = bytes_equals(digest.digest, s1_digest, 32);

        return ret;
    }

    int c1s1_strategy::calc_c1_digest(c1s1* owner, char*& c1_digest)
    {
        int ret = ERROR_SUCCESS;

        /**
         * c1s1 is splited by digest:
         *     c1s1-part1: n bytes (time, version, key and digest-part1).
         *     digest-data: 32bytes
         *     c1s1-part2: (1536-n-32)bytes (digest-part2)
         * @return a new allocated bytes, user must free it.
         */
        char* c1s1_joined_bytes = new char[1536 -32];
        DAutoFreeArray(char, c1s1_joined_bytes);
        if ((ret = copy_to(owner, c1s1_joined_bytes, 1536 - 32, false)) != ERROR_SUCCESS) {
            return ret;
        }

        c1_digest = new char[SRS_OpensslHashSize];
        if ((ret = openssl_HMACsha256(SrsGenuineFPKey, 30, c1s1_joined_bytes, 1536 - 32, c1_digest)) != ERROR_SUCCESS) {
            DFreeArray(c1_digest);
            log_error("calc digest for c1 failed. ret=%d", ret);
            return ret;
        }
        log_verbose("digest calculated for c1");

        return ret;
    }

    int c1s1_strategy::calc_s1_digest(c1s1* owner, char*& s1_digest)
    {
        int ret = ERROR_SUCCESS;

        /**
         * c1s1 is splited by digest:
         *     c1s1-part1: n bytes (time, version, key and digest-part1).
         *     digest-data: 32bytes
         *     c1s1-part2: (1536-n-32)bytes (digest-part2)
         * @return a new allocated bytes, user must free it.
         */
        char* c1s1_joined_bytes = new char[1536 -32];
        DAutoFreeArray(char, c1s1_joined_bytes);
        if ((ret = copy_to(owner, c1s1_joined_bytes, 1536 - 32, false)) != ERROR_SUCCESS) {
            return ret;
        }

        s1_digest = new char[SRS_OpensslHashSize];
        if ((ret = openssl_HMACsha256(SrsGenuineFMSKey, 36, c1s1_joined_bytes, 1536 - 32, s1_digest)) != ERROR_SUCCESS) {
            DFreeArray(s1_digest);
            log_error("calc digest for s1 failed. ret=%d", ret);
            return ret;
        }
        log_verbose("digest calculated for s1");

        return ret;
    }

    void c1s1_strategy::copy_time_version(DStream* stream, c1s1* owner)
    {
        DAssert(stream->left() >= 8);

        // 4bytes time
        stream->write4Bytes(owner->time);

        // 4bytes version
        stream->write4Bytes(owner->version);
    }
    void c1s1_strategy::copy_key(DStream* stream)
    {
        DAssert(key.random0_size >= 0);
        DAssert(key.random1_size >= 0);

        int total = key.random0_size + 128 + key.random1_size + 4;
        DAssert(stream->left() >= total);

        // 764bytes key block
        if (key.random0_size > 0) {
            stream->writeBytes(key.random0, key.random0_size);
        }

        stream->writeBytes(key.key, 128);

        if (key.random1_size > 0) {
            stream->writeBytes(key.random1, key.random1_size);
        }

        stream->write4Bytes(key.offset);
    }
    void c1s1_strategy::copy_digest(DStream* stream, bool with_digest)
    {
        DAssert(key.random0_size >= 0);
        DAssert(key.random1_size >= 0);

        int total = 4 + digest.random0_size + digest.random1_size;
        if (with_digest) {
            total += 32;
        }
        DAssert(stream->left() >= total);

        // 732bytes digest block without the 32bytes digest-data
        // nbytes digest block part1
        stream->write4Bytes(digest.offset);

        // digest random padding.
        if (digest.random0_size > 0) {
            stream->writeBytes(digest.random0, digest.random0_size);
        }

        // digest
        if (with_digest) {
            stream->writeBytes(digest.digest, 32);
        }

        // nbytes digest block part2
        if (digest.random1_size > 0) {
            stream->writeBytes(digest.random1, digest.random1_size);
        }
    }

    c1s1_strategy_schema0::c1s1_strategy_schema0()
    {
    }

    c1s1_strategy_schema0::~c1s1_strategy_schema0()
    {
    }

    srs_schema_type c1s1_strategy_schema0::schema()
    {
        return srs_schema0;
    }

    int c1s1_strategy_schema0::parse(char* _c1s1, int size)
    {
        int ret = ERROR_SUCCESS;

        DAssert(size == 1536);

        DStream stream(_c1s1 + 8, 764);

        if ((ret = key.parse(&stream)) != ERROR_SUCCESS) {
            log_error("parse the c1 key failed. ret=%d", ret);
            return ret;
        }

        DStream stream1(_c1s1 + 8 + 764, 764);

        if ((ret = digest.parse(&stream1)) != ERROR_SUCCESS) {
            log_error("parse the c1 digest failed. ret=%d", ret);
            return ret;
        }

        log_verbose("parse c1 key-digest success");

        return ret;
    }

    int c1s1_strategy_schema0::copy_to(c1s1* owner, char* bytes, int size, bool with_digest)
    {
        int ret = ERROR_SUCCESS;

        if (with_digest) {
            DAssert(size == 1536);
        } else {
            DAssert(size == 1504);
        }

        DStream stream(bytes, size);

        copy_time_version(&stream, owner);
        copy_key(&stream);
        copy_digest(&stream, with_digest);

        DAssert(stream.size() != 0);

        return ret;
    }

    c1s1_strategy_schema1::c1s1_strategy_schema1()
    {
    }

    c1s1_strategy_schema1::~c1s1_strategy_schema1()
    {
    }

    srs_schema_type c1s1_strategy_schema1::schema()
    {
        return srs_schema1;
    }

    int c1s1_strategy_schema1::parse(char* _c1s1, int size)
    {
        int ret = ERROR_SUCCESS;

        DAssert(size == 1536);

        DStream stream(_c1s1 + 8, 764);

        if ((ret = digest.parse(&stream)) != ERROR_SUCCESS) {
            log_error("parse the c1 digest failed. ret=%d", ret);
            return ret;
        }

        DStream stream1(_c1s1 + 8 + 764, 764);

        if ((ret = key.parse(&stream1)) != ERROR_SUCCESS) {
            log_error("parse the c1 key failed. ret=%d", ret);
            return ret;
        }

        log_verbose("parse c1 digest-key success");

        return ret;
    }

    int c1s1_strategy_schema1::copy_to(c1s1* owner, char* bytes, int size, bool with_digest)
    {
        int ret = ERROR_SUCCESS;

        if (with_digest) {
            DAssert(size == 1536);
        } else {
            DAssert(size == 1504);
        }

        DStream stream(bytes, size);

        copy_time_version(&stream, owner);
        copy_digest(&stream, with_digest);
        copy_key(&stream);

        DAssert(stream.size() != 0);

        return ret;
    }

    c1s1::c1s1()
    {
        payload = NULL;
    }
    c1s1::~c1s1()
    {
        DFree(payload);
    }

    srs_schema_type c1s1::schema()
    {
        DAssert(payload != NULL);
        return payload->schema();
    }

    char* c1s1::get_digest()
    {
        DAssert(payload != NULL);
        return payload->get_digest();
    }

    char* c1s1::get_key()
    {
        DAssert(payload != NULL);
        return payload->get_key();
    }

    int c1s1::dump(char* _c1s1, int size)
    {
        DAssert(size == 1536);
        DAssert(payload != NULL);
        return payload->dump(this, _c1s1, size);
    }

    int c1s1::parse(char* _c1s1, int size, srs_schema_type schema)
    {
        int ret = ERROR_SUCCESS;

        DAssert(size == 1536);

        if (schema != srs_schema0 && schema != srs_schema1) {
            ret = ERROR_RTMP_CH_SCHEMA;
            log_error("parse c1 failed. invalid schema=%d, ret=%d", schema, ret);
            return ret;
        }

        DStream stream(_c1s1, size);

        stream.read4Bytes(time);
        stream.read4Bytes(version); // client c1 version

        DFree(payload);
        if (schema == srs_schema0) {
            payload = new c1s1_strategy_schema0();
        } else {
            payload = new c1s1_strategy_schema1();
        }

        return payload->parse(_c1s1, size);
    }

    int c1s1::c1_create(srs_schema_type schema)
    {
        int ret = ERROR_SUCCESS;

        if (schema != srs_schema0 && schema != srs_schema1) {
            ret = ERROR_RTMP_CH_SCHEMA;
            log_error("create c1 failed. invalid schema=%d, ret=%d", schema, ret);
            return ret;
        }

        // client c1 time and version
        time = (int32_t)::time(NULL);
        version = 0x80000702; // client c1 version

        // generate signature by schema
        DFree(payload);
        if (schema == srs_schema0) {
            payload = new c1s1_strategy_schema0();
        } else {
            payload = new c1s1_strategy_schema1();
        }

        return payload->c1_create(this);
    }

    int c1s1::c1_validate_digest(bool& is_valid)
    {
        is_valid = false;
        DAssert(payload);
        return payload->c1_validate_digest(this, is_valid);
    }

    int c1s1::s1_create(c1s1* c1)
    {
        int ret = ERROR_SUCCESS;

        if (c1->schema() != srs_schema0 && c1->schema() != srs_schema1) {
            ret = ERROR_RTMP_CH_SCHEMA;
            log_error("create s1 failed. invalid schema=%d, ret=%d", c1->schema(), ret);
            return ret;
        }

        time = ::time(NULL);
        version = 0x01000504; // server s1 version

        DFree(payload);
        if (c1->schema() == srs_schema0) {
            payload = new c1s1_strategy_schema0();
        } else {
            payload = new c1s1_strategy_schema1();
        }

        return payload->s1_create(this, c1);
    }

    int c1s1::s1_validate_digest(bool& is_valid)
    {
        is_valid = false;
        DAssert(payload);
        return payload->s1_validate_digest(this, is_valid);
    }

    c2s2::c2s2()
    {
        random_generate(random, 1504);
        random_generate(digest, 32);
    }

    c2s2::~c2s2()
    {
    }

    int c2s2::dump(char* _c2s2, int size)
    {
        DAssert(size == 1536);

        memcpy(_c2s2, random, 1504);
        memcpy(_c2s2 + 1504, digest, 32);

        return ERROR_SUCCESS;
    }

    int c2s2::parse(char* _c2s2, int size)
    {
        DAssert(size == 1536);

        memcpy(random, _c2s2, 1504);
        memcpy(digest, _c2s2 + 1504, 32);

        return ERROR_SUCCESS;
    }

    int c2s2::c2_create(c1s1* s1)
    {
        int ret = ERROR_SUCCESS;

        char temp_key[SRS_OpensslHashSize];
        if ((ret = openssl_HMACsha256(SrsGenuineFPKey, 62, s1->get_digest(), 32, temp_key)) != ERROR_SUCCESS) {
            log_error("create c2 temp key failed. ret=%d", ret);
            return ret;
        }
        log_verbose("generate c2 temp key success.");

        char _digest[SRS_OpensslHashSize];
        if ((ret = openssl_HMACsha256(temp_key, 32, random, 1504, _digest)) != ERROR_SUCCESS) {
            log_error("create c2 digest failed. ret=%d", ret);
            return ret;
        }
        log_verbose("generate c2 digest success.");

        memcpy(digest, _digest, 32);

        return ret;
    }

    int c2s2::c2_validate(c1s1* s1, bool& is_valid)
    {
        is_valid = false;
        int ret = ERROR_SUCCESS;

        char temp_key[SRS_OpensslHashSize];
        if ((ret = openssl_HMACsha256(SrsGenuineFPKey, 62, s1->get_digest(), 32, temp_key)) != ERROR_SUCCESS) {
            log_error("create c2 temp key failed. ret=%d", ret);
            return ret;
        }
        log_verbose("generate c2 temp key success.");

        char _digest[SRS_OpensslHashSize];
        if ((ret = openssl_HMACsha256(temp_key, 32, random, 1504, _digest)) != ERROR_SUCCESS) {
            log_error("create c2 digest failed. ret=%d", ret);
            return ret;
        }
        log_verbose("generate c2 digest success.");

        is_valid = bytes_equals(digest, _digest, 32);

        return ret;
    }

    int c2s2::s2_create(c1s1* c1)
    {
        int ret = ERROR_SUCCESS;

        char temp_key[SRS_OpensslHashSize];
        if ((ret = openssl_HMACsha256(SrsGenuineFMSKey, 68, c1->get_digest(), 32, temp_key)) != ERROR_SUCCESS) {
            log_error("create s2 temp key failed. ret=%d", ret);
            return ret;
        }
        log_verbose("generate s2 temp key success.");

        char _digest[SRS_OpensslHashSize];
        if ((ret = openssl_HMACsha256(temp_key, 32, random, 1504, _digest)) != ERROR_SUCCESS) {
            log_error("create s2 digest failed. ret=%d", ret);
            return ret;
        }
        log_verbose("generate s2 digest success.");

        memcpy(digest, _digest, 32);

        return ret;
    }

    int c2s2::s2_validate(c1s1* c1, bool& is_valid)
    {
        is_valid = false;
        int ret = ERROR_SUCCESS;

        char temp_key[SRS_OpensslHashSize];
        if ((ret = openssl_HMACsha256(SrsGenuineFMSKey, 68, c1->get_digest(), 32, temp_key)) != ERROR_SUCCESS) {
            log_error("create s2 temp key failed. ret=%d", ret);
            return ret;
        }
        log_verbose("generate s2 temp key success.");

        char _digest[SRS_OpensslHashSize];
        if ((ret = openssl_HMACsha256(temp_key, 32, random, 1504, _digest)) != ERROR_SUCCESS) {
            log_error("create s2 digest failed. ret=%d", ret);
            return ret;
        }
        log_verbose("generate s2 digest success.");

        is_valid = bytes_equals(digest, _digest, 32);

        return ret;
    }
}

/******************************************************************************/

rtmp_handshake::rtmp_handshake(DTcpSocket *socket)
    : m_socket(socket)
    , m_type(ComplexC0C1)
{

}

rtmp_handshake::~rtmp_handshake()
{

}

int rtmp_handshake::handshake_with_client()
{
    int ret = ERROR_SUCCESS;

    switch (m_type) {
    case ComplexC0C1:
        ret = parse_complex_c0c1();
        break;
    case ComplexS0S1S2:
        ret = parse_complex_s0s1s2();
        break;
    case ComplexC2:
        ret = parse_complex_c2();
        break;
    case SimpleC0C1:
        ret = parse_simple_c0c1();
        break;
    case SimpleS0S1S2:
        ret = parse_simple_s0s1s2();
        break;
    case SimpleC2:
        ret = parse_simple_c2();
        break;
    case Completed:
        break;
    default:
        break;
    }

    return ret;
}

int rtmp_handshake::handshake_with_server()
{
    int ret = ERROR_SUCCESS;

    switch (m_type) {
    case ComplexC0C1:
        ret = process_complex_c0c1();
        break;
    case ComplexS0S1S2:
        ret = process_complex_s0s1s2();
        break;
    case ComplexC2:
        ret = process_complex_c2();
        break;
    case SimpleC0C1:
        ret = process_simple_c0c1();
        break;
    case SimpleS0S1S2:
        ret = process_simple_s0s1s2();
        break;
    case SimpleC2:
        ret = process_simple_c2();
        break;
    case Completed:
        break;
    default:
        break;
    }

    return ret;
}

void rtmp_handshake::set_handshake_with_server_type(bool complex)
{
    if (complex) {
        m_type = ComplexC0C1;
    } else {
        m_type = SimpleC0C1;
    }
}

bool rtmp_handshake::completed()
{
    return m_type == Completed;
}

int rtmp_handshake::parse_complex_c0c1()
{
    int ret = ERROR_SUCCESS;

    if (!c0c1.get()) {
        MemoryChunk *chunk = DMemPool::instance()->getMemory(1537);
        c0c1 = DSharedPtr<MemoryChunk>(chunk);
        c0c1->length = 1537;
    }

    if ((ret = m_socket->read(c0c1->data, 1537)) != ERROR_SUCCESS) {
        log_error_eagain(ret, ERROR_HANDSHAKE_READ_C0C1, "read c0c1 failed. ret=%d", ret);
        return ret;
    }

    // decode c1
    c1s1 c1;
    // try schema0.
    // @remark, use schema0 to make flash player happy.
    if ((ret = c1.parse(c0c1->data + 1, 1536, srs_schema0)) != ERROR_SUCCESS) {
        log_error("parse c1 schema%d error. ret=%d", srs_schema0, ret);
        return ret;
    }

    // try schema1
    bool is_valid = false;
    if ((ret = c1.c1_validate_digest(is_valid)) != ERROR_SUCCESS || !is_valid) {
        log_info("schema0 failed, try schema1.");
        if ((ret = c1.parse(c0c1->data + 1, 1536, srs_schema1)) != ERROR_SUCCESS) {
            log_error("parse c1 schema%d error. ret=%d", srs_schema1, ret);
            return ret;
        }

        if ((ret = c1.c1_validate_digest(is_valid)) != ERROR_SUCCESS || !is_valid) {
            m_type = SimpleC0C1;
            log_info("all schema valid failed, try simple handshake. ret=%d", ret);
            return ERROR_SUCCESS;
        }
    } else {
        log_info("schema0 is ok.");
    }
    log_verbose("decode c1 success.");

    // encode s1
    if ((ret = s1.s1_create(&c1)) != ERROR_SUCCESS) {
        log_error("create s1 from c1 failed. ret=%d", ret);
        return ret;
    }
    log_verbose("create s1 from c1 success.");
    // verify s1
    if ((ret = s1.s1_validate_digest(is_valid)) != ERROR_SUCCESS || !is_valid) {
        m_type = SimpleC0C1;
        log_info("verify s1 failed, try simple handshake. ret=%d", ret);
        return ERROR_SUCCESS;
    }
    log_verbose("verify s1 success.");

    if ((ret = s2.s2_create(&c1)) != ERROR_SUCCESS) {
        log_error("create s2 from c1 failed. ret=%d", ret);
        return ret;
    }
    log_verbose("create s2 from c1 success.");
    // verify s2
    if ((ret = s2.s2_validate(&c1, is_valid)) != ERROR_SUCCESS || !is_valid) {
        m_type = SimpleC0C1;
        log_info("verify s2 failed, try simple handshake. ret=%d", ret);
        return ERROR_SUCCESS;
    }
    log_verbose("verify s2 success.");

    m_type = ComplexS0S1S2;

    return ret;
}

int rtmp_handshake::parse_complex_s0s1s2()
{
    int ret = ERROR_SUCCESS;

    create_s0s1s2();

    if ((ret = s1.dump(s0s1s2->data + 1, 1536)) != ERROR_SUCCESS) {
        return ret;
    }
    if ((ret = s2.dump(s0s1s2->data + 1537, 1536)) != ERROR_SUCCESS) {
        return ret;
    }

    m_type = ComplexC2;

    if ((ret = m_socket->write(s0s1s2, 3073)) != ERROR_SUCCESS) {
        log_error_eagain(ret, ERROR_HANDSHAKE_WRITE_S0S1S2, "write s0s1s2 failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int rtmp_handshake::parse_complex_c2()
{
    int ret = ERROR_SUCCESS;

    if (!c2.get()) {
        MemoryChunk *chunk = DMemPool::instance()->getMemory(1536);
        c2 = DSharedPtr<MemoryChunk>(chunk);
        c2->length = 1536;
    }

    if ((ret = m_socket->read(c2->data, 1536)) != ERROR_SUCCESS) {
        log_error_eagain(ret, ERROR_HANDSHAKE_READ_C2, "read c2 failed. ret=%d", ret);
        return ret;
    }

    c2s2 _c2;
    if ((ret = _c2.parse(c2->data, 1536)) != ERROR_SUCCESS) {
        return ret;
    }
    log_verbose("complex handshake read c2 success.");

    // verify c2
    // never verify c2, for ffmpeg will failed.
    // it's ok for flash.

    log_trace("complex handshake success");

    m_type = Completed;

    return ret;
}

int rtmp_handshake::parse_simple_c0c1()
{
    int ret = ERROR_SUCCESS;

    if (!c0c1.get()) {
        MemoryChunk *chunk = DMemPool::instance()->getMemory(1537);
        c0c1 = DSharedPtr<MemoryChunk>(chunk);
        c0c1->length = 1537;

        if ((ret = m_socket->read(c0c1->data, 1537)) != ERROR_SUCCESS) {
            log_error_eagain(ret, ERROR_HANDSHAKE_READ_C0C1, "read c0c1 failed. ret=%d", ret);
            return ret;
        }
    }

    // plain text required.
    char *c0c1_data = c0c1->data;
    if (c0c1_data[0] != 0x03) {
        ret = ERROR_HANDSHAKE_PLAIN_REQUIRED;
        log_warn("only support rtmp plain text. ret=%d", ret);
        return ret;
    }
    log_verbose("check c0 success, required plain text.");

    m_type = SimpleS0S1S2;

    return ret;
}

int rtmp_handshake::parse_simple_s0s1s2()
{
    int ret = ERROR_SUCCESS;

    create_s0s1s2(c0c1->data + 1);

    m_type = SimpleC2;

    if ((ret = m_socket->write(s0s1s2, 3073)) != ERROR_SUCCESS) {
        log_error_eagain(ret, ERROR_HANDSHAKE_WRITE_S0S1S2, "write s0s1s2 failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int rtmp_handshake::parse_simple_c2()
{
    int ret = ERROR_SUCCESS;

    if (!c2.get()) {
        MemoryChunk *chunk = DMemPool::instance()->getMemory(1536);
        c2 = DSharedPtr<MemoryChunk>(chunk);
        c2->length = 1536;
    }

    if ((ret = m_socket->read(c2->data, 1536)) != ERROR_SUCCESS) {
        log_error_eagain(ret, ERROR_HANDSHAKE_READ_C2, "read c2 failed. ret=%d", ret);
        return ret;
    }

    m_type = Completed;

    return ret;
}

void rtmp_handshake::create_s0s1s2(const char *_c1)
{
    if (s0s1s2.get()) {
        return;
    }

    MemoryChunk *chunk = DMemPool::instance()->getMemory(3073);
    s0s1s2 = DSharedPtr<MemoryChunk>(chunk);
    s0s1s2->length = 3073;

    random_generate(s0s1s2->data, 3073);

    // plain text required.
    DStream stream(s0s1s2->data, 9);

    stream.write1Bytes(0x03);
    stream.write4Bytes((dint32)::time(NULL));
    // s2 time2 copy from c1
    if (c0c1.get()) {
        stream.writeBytes(c0c1->data + 1, 4);
    }

    // if c1 specified, copy c1 to s2.
    if (_c1) {
        memcpy(s0s1s2->data + 1537, _c1, 1536);
    }
}

void rtmp_handshake::create_c0c1()
{
    if (c0c1.get()) {
        return;
    }

    MemoryChunk *chunk = DMemPool::instance()->getMemory(1537);
    c0c1 = DSharedPtr<MemoryChunk>(chunk);
    c0c1->length = 1537;

    random_generate(c0c1->data, 1537);

    // plain text required.
    DStream stream;

    stream.write1Bytes(0x03);
    stream.write4Bytes((int32_t)::time(NULL));
    stream.write4Bytes(0x00);

    memcpy(c0c1->data, stream.data(), 9);
}

void rtmp_handshake::create_c2()
{
    if (c2.get()) {
        return;
    }

    MemoryChunk *chunk = DMemPool::instance()->getMemory(1536);
    c2 = DSharedPtr<MemoryChunk>(chunk);
    c2->length = 1536;

    random_generate(c2->data, 1536);

    // time
    DStream stream;

    stream.write4Bytes((int32_t)::time(NULL));
    // c2 time2 copy from s1
    if (s0s1s2.get()) {
        stream.writeBytes(s0s1s2->data + 1, 4);
    }

    memcpy(c2->data, stream.data(), 8);
}

int rtmp_handshake::process_complex_c0c1()
{
    int ret = ERROR_SUCCESS;

    create_c0c1();

    // @remark, FMS requires the schema1(digest-key), or connect failed.
    if ((ret = c1.c1_create(srs_schema1)) != ERROR_SUCCESS) {
        return ret;
    }
    if ((ret = c1.dump(c0c1->data + 1, 1536)) != ERROR_SUCCESS) {
        return ret;
    }

    // verify c1
    bool is_valid;
    if ((ret = c1.c1_validate_digest(is_valid)) != ERROR_SUCCESS || !is_valid) {
        m_type = SimpleC0C1;
        return ret;
    }

    m_type = ComplexS0S1S2;

    if ((ret = m_socket->write(c0c1, 1537)) != ERROR_SUCCESS) {
        log_error_eagain(ret, ERROR_HANDSHAKE_WRITE_C0C1, "write c0c1 failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int rtmp_handshake::process_complex_s0s1s2()
{
    int ret = ERROR_SUCCESS;

    if (!s0s1s2.get()) {
        MemoryChunk *chunk = DMemPool::instance()->getMemory(3073);
        s0s1s2 = DSharedPtr<MemoryChunk>(chunk);
        s0s1s2->length = 3073;
    }

    if ((ret = m_socket->read(s0s1s2->data, 3073)) != ERROR_SUCCESS) {
        log_error_eagain(ret, ERROR_HANDSHAKE_READ_S0S1S2, "read s0s1s2 failed. ret=%d", ret);
        return ret;
    }

    // plain text required.
    char *s0s1s2_data = s0s1s2->data;
    if (s0s1s2_data[0] != 0x03) {
        ret = ERROR_HANDSHAKE_PLAIN_REQUIRED;
        log_warn("handshake failed, plain text required. ret=%d", ret);
        return ret;
    }

    // verify s1s2
    if ((ret = s1.parse(s0s1s2->data + 1, 1536, c1.schema())) != ERROR_SUCCESS) {
        return ret;
    }

    m_type = ComplexC2;

    return ret;
}

int rtmp_handshake::process_complex_c2()
{
    int ret = ERROR_SUCCESS;

    create_c2();

    c2s2 _c2;
    if ((ret = _c2.c2_create(&s1)) != ERROR_SUCCESS) {
        return ret;
    }

    if ((ret = _c2.dump(c2->data, 1536)) != ERROR_SUCCESS) {
        return ret;
    }

    m_type = Completed;

    if ((ret = m_socket->write(c2, 1536)) != ERROR_SUCCESS) {
        log_error_eagain(ret, ERROR_HANDSHAKE_WRITE_C2, "write c2 failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int rtmp_handshake::process_simple_c0c1()
{
    int ret = ERROR_SUCCESS;

    create_c0c1();

    m_type = SimpleS0S1S2;

    if ((ret = m_socket->write(c0c1, 1537)) != ERROR_SUCCESS) {
        log_error_eagain(ret, ERROR_HANDSHAKE_WRITE_C0C1, "write c0c1 failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int rtmp_handshake::process_simple_s0s1s2()
{
    int ret = ERROR_SUCCESS;

    if (!s0s1s2.get()) {
        MemoryChunk *chunk = DMemPool::instance()->getMemory(3073);
        s0s1s2 = DSharedPtr<MemoryChunk>(chunk);
        s0s1s2->length = 3073;
    }

    if ((ret = m_socket->read(s0s1s2->data, 3073)) != ERROR_SUCCESS) {
        log_error_eagain(ret, ERROR_HANDSHAKE_READ_S0S1S2, "read s0s1s2 failed. ret=%d", ret);
        return ret;
    }

    // plain text required.
    char *s0s1s2_data = s0s1s2->data;
    if (s0s1s2_data[0] != 0x03) {
        ret = ERROR_HANDSHAKE_PLAIN_REQUIRED;
        log_warn("handshake failed, plain text required. ret=%d", ret);
        return ret;
    }

    m_type = SimpleC2;

    return ret;
}

int rtmp_handshake::process_simple_c2()
{
    int ret = ERROR_SUCCESS;

    create_c2();

    memcpy(c2->data, s0s1s2->data + 1, 1536);

    m_type = Completed;

    if ((ret = m_socket->write(c2, 1536)) != ERROR_SUCCESS) {
        log_error_eagain(ret, ERROR_HANDSHAKE_WRITE_C2, "write c2 failed. ret=%d", ret);
        return ret;
    }

    return ret;
}
