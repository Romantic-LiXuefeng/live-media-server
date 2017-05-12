#ifndef KERNEL_ERRNO_HPP
#define KERNEL_ERRNO_HPP

#define ERROR_SUCCESS           0

/************************ rtmp handshake *************************/
#define ERROR_OpenSslCreateDH               1001
#define ERROR_OpenSslCreateP                1002
#define ERROR_OpenSslCreateG                1003
#define ERROR_OpenSslParseP1024             1004
#define ERROR_OpenSslSetG                   1005
#define ERROR_OpenSslGenerateDHKeys         1006
#define ERROR_OpenSslCopyKey                1007
#define ERROR_OpenSslSha256Update           1008
#define ERROR_OpenSslSha256Init             1009
#define ERROR_OpenSslSha256Final            1010
#define ERROR_OpenSslSha256EvpDigest        1011
#define ERROR_OpenSslSha256DigestSize       1012
#define ERROR_OpenSslGetPeerPublicKey       1013
#define ERROR_OpenSslComputeSharedKey       1014
#define ERROR_HANDSHAKE_CH_SCHEMA           1005
#define ERROR_HANDSHAKE_PLAIN_REQUIRED      1016
#define ERROR_HANDSHAKE_TRY_SIMPLE_HS       1017
#define ERROR_HANDSHAKE_READ_C0C1           1018
#define ERROR_HANDSHAKE_READ_S0S1S2         1019
#define ERROR_HANDSHAKE_READ_C2             1020
#define ERROR_KEY_BLOCK_INVALID_OFFSET      1021
#define ERROR_DIGEST_BLOCK_INVALID_OFFSET   1022

/************************ rtmp chunk *************************/
#define ERROR_RTMP_CHUNK_START              1023
#define ERROR_RTMP_PACKET_SIZE              1024
#define ERROR_RTMP_CHUNK_HEADER_NEGATIVE    1025

/************************ rtmp amf *************************/
#define ERROR_RTMP_AMF0_NUMBER_DECODE       1030
#define ERROR_RTMP_AMF0_STRING_DECODE       1031
#define ERROR_RTMP_AMF0_OBJECT_DECODE       1032
#define ERROR_RTMP_AMF0_BOOL_DECODE         1033
#define ERROR_RTMP_AMF0_NULL_DECODE         1034
#define ERROR_RTMP_AMF0_UNDEFINED_DECODE    1035
#define ERROR_RTMP_AMF0_ECMAARRAY_DECODE    1036
#define ERROR_RTMP_AMF0_STRICTARRAY_DECODE  1037
#define ERROR_RTMP_AMF0_DATE_DECODE         1038
#define ERROR_RTMP_AMF0_ANY_DECODE          1039

#define ERROR_RTMP_AMF0_NUMBER_ENCODE       1040
#define ERROR_RTMP_AMF0_STRING_ENCODE       1041
#define ERROR_RTMP_AMF0_OBJECT_ENCODE       1042
#define ERROR_RTMP_AMF0_BOOL_ENCODE         1043
#define ERROR_RTMP_AMF0_NULL_ENCODE         1044
#define ERROR_RTMP_AMF0_UNDEFINED_ENCODE    1045
#define ERROR_RTMP_AMF0_ECMAARRAY_ENCODE    1046
#define ERROR_RTMP_AMF0_STRICTARRAY_ENCODE  1047
#define ERROR_RTMP_AMF0_DATE_ENCODE         1048
#define ERROR_RTMP_AMF0_ANY_ENCODE          1049

/************************ rtmp decode/encode *********************/
#define ERROR_RTMP_COMMAND_NAME_NOTFOUND    1050
#define ERROR_RTMP_COMMAND_NAME_INVALID     1051
#define ERROR_RTMP_DECODE_VALUE_NOTFOUND    1052
#define ERROR_RTMP_TRANSACTION_ID_NOTFOUND  1053
#define ERROR_RTMP_TRANSACTION_ID_INVALID   1054
#define ERROR_RTMP_STREAM_ID_NOTFOUND       1055
#define ERROR_RTMP_STREAM_NAME_NOTFOUND     1056
#define ERROR_RTMP_STREAM_NAME_INVALID      1057
#define ERROR_RTMP_MESSAGE_DECODE           1058

#define ERROR_RTMP_CONNECT_REFUSED          1059
#define ERROR_RTMP_CREATE_STREAM_REFUSED    1060
#define ERROR_RTMP_PUBLISH_REFUSED          1061
#define ERROR_RTMP_PLAY_REFUSED             1062

#define ERROR_RTMP_AGGREGATE                1063

/************************* tcp socket ****************************/
#define ERROR_TCP_SOCKET_READ_FAILED        1100
#define ERROR_TCP_SOCKET_COPY_FAILED        1101
#define ERROR_TCP_SOCKET_WRITE_FAILED       1102
#define ERROR_TCP_SOCKET_CONNECT            1103

#define ERROR_KERNEL_BUFFER_NOT_ENOUGH      1104

/*************************************************************/
#define ERROR_CONFIG_INVALID                1200
#define ERROR_CONFIG_DIRECTIVE              1201
#define ERROR_CONFIG_BLOCK_START            1202
#define ERROR_CONFIG_BLOCK_END              1203
#define ERROR_CONFIG_EOF                    1204
#define ERROR_CONFIG_LOAD_FILE              1207
#define ERROR_PROXY_PASS                    1208
#define ERROR_SERVER_CONFIG_EMPTY           1209

/************************* http ****************************/
#define ERROR_HTTP_HEADER_PARSER            1301
#define ERROR_HTTP_INVALID_CHUNK_HEADER     1302
#define ERROR_HTTP_READ_EOF                 1303

#define ERROR_FLV_READ_HEADER               1304
#define ERROR_FLV_HEADER_TYPE               1305
#define ERROR_FLV_READ_TAG_HEADER           1306
#define ERROR_FLV_READ_TAG_BODY             1307
#define ERROR_FLV_READ_PREVIOUS_TAG_SIZE    1308

#define ERROR_OPEN_FILE                     1309
#define ERROR_FILE_NOT_EXIST                1310
#define ERROR_SEND_FILE                     1311

#define ERROR_REFERER_VERIFY                1312

#define ERROR_HTTP_PARSE_URI                1313
#define ERROR_HTTP_READ_BODY                1314
#define ERROR_RESPONSE_CODE                 1315

/************************* server ****************************/
#define ERROR_GENERATE_REQUEST              1400
#define ERROR_GET_CONFIG_VALUE              1401
#define ERROR_EGDE_GET_IP_PORT              1402
#define ERROR_SERVER_PROXY_MESSAGE          1403

#define ERROR_SOURCE_ONPLAY                 1404
#define ERROR_SOURCE_ONPUBLISH              1405

#endif // KERNEL_ERRNO_HPP
