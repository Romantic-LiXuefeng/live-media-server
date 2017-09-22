#ifndef KERNEL_ERRNO_HPP
#define KERNEL_ERRNO_HPP

#define ERROR_SUCCESS                       0

/************************ rtmp handshake *************************/
#define ERROR_OpenSslCreateDH               1001
#define ERROR_OpenSslCreateP                1002
#define ERROR_OpenSslCreateG                1003
#define ERROR_OpenSslParseP1024             1004
#define ERROR_OpenSslSetG                   1005
#define ERROR_OpenSslGenerateDHKeys         1006
#define ERROR_OpenSslSha256Update           1007
#define ERROR_OpenSslSha256Init             1008
#define ERROR_OpenSslSha256Final            1009
#define ERROR_OpenSslSha256EvpDigest        1010
#define ERROR_OpenSslSha256DigestSize       1011
#define ERROR_OpenSslGetPeerPublicKey       1012
#define ERROR_OpenSslComputeSharedKey       1013
#define ERROR_OpenSslCreateHMAC             1014
#define ERROR_RTMP_CH_SCHEMA                1015

#define ERROR_HANDSHAKE_PLAIN_REQUIRED      1016

#define ERROR_HANDSHAKE_READ_C0C1           1017
#define ERROR_HANDSHAKE_WRITE_S0S1S2        1018
#define ERROR_HANDSHAKE_READ_C2             1019
#define ERROR_HANDSHAKE_WRITE_C0C1          1020
#define ERROR_HANDSHAKE_READ_S0S1S2         1021
#define ERROR_HANDSHAKE_WRITE_C2            1022

/************************ rtmp chunk *************************/
#define ERROR_RTMP_CHUNK_START              1023
#define ERROR_RTMP_PACKET_SIZE              1024
#define ERROR_RTMP_CHUNK_HEADER_NEGATIVE    1025

#define ERROR_CHUNK_READ_BASIC_HEADER       1026
#define ERROR_CHUNK_READ_MESSAGE_HEADER     1027
#define ERROR_CHUNK_READ_EXTEND_TIMESTAMP   1028
#define ERROR_CHUNK_COPY_EXTEND_TIMESTAMP   1029
#define ERROR_CHUNK_READ_PAYLOAD            1030
#define ERROR_CHUNK_WRITE_HEADER            1031
#define ERROR_CHUNK_WRITE_PAYLOAD           1032

#define ERROR_HTTP_COPY_CHUNK_HEADER        1033
#define ERROR_HTTP_READ_CHUNK_BEGIN_CRLF    1034
#define ERROR_HTTP_READ_CHUNK_BODY          1035
#define ERROR_HTTP_READ_CHUNK_END_CRLF      1036

#define ERROR_HTTP_WRITE_CHUNK_HEADER       1037
#define ERROR_HTTP_WRITE_CHUNK_PAYLOAD      1038
#define ERROR_HTTP_WRITE_CHUNK_PRE_TAG      1039
#define ERROR_HTTP_WRITE_FLV_HEADER         1040

#define ERROR_RTMP_CHUNK_WRITE              1041

/************************ rtmp amf *************************/
#define ERROR_RTMP_AMF0_ANY_DECODE          1042

#define ERROR_RTMP_AMF0_NUMBER_ENCODE       1043
#define ERROR_RTMP_AMF0_STRING_ENCODE       1044
#define ERROR_RTMP_AMF0_OBJECT_ENCODE       1045
#define ERROR_RTMP_AMF0_BOOL_ENCODE         1046
#define ERROR_RTMP_AMF0_NULL_ENCODE         1047
#define ERROR_RTMP_AMF0_UNDEFINED_ENCODE    1048
#define ERROR_RTMP_AMF0_ECMAARRAY_ENCODE    1049
#define ERROR_RTMP_AMF0_ANY_ENCODE          1050

/************************ rtmp decode/encode *********************/
#define ERROR_RTMP_COMMAND_NAME_NOTFOUND    1051
#define ERROR_RTMP_COMMAND_NAME_INVALID     1052
#define ERROR_RTMP_DECODE_VALUE_NOTFOUND    1053
#define ERROR_RTMP_TRANSACTION_ID_NOTFOUND  1054
#define ERROR_RTMP_TRANSACTION_ID_INVALID   1055
#define ERROR_RTMP_STREAM_ID_NOTFOUND       1056
#define ERROR_RTMP_STREAM_NAME_NOTFOUND     1057
#define ERROR_RTMP_STREAM_NAME_INVALID      1058
#define ERROR_RTMP_MESSAGE_DECODE           1059

#define ERROR_RTMP_CONNECT_REFUSED          1060
#define ERROR_RTMP_PUBLISH_REFUSED          1061
#define ERROR_RTMP_PLAY_REFUSED             1062
#define ERROR_RTMP_PLAY_START_FAILED        1063

#define ERROR_RTMP_AGGREGATE                1064

#define ERROR_RTMP_INVALID_ORDER            1065

#define ERROR_RTMP_CLIENT_PUBLISH_START     1066
#define ERROR_RTMP_CLIENT_PLAY_START        1067

/************************* tcp socket ****************************/
#define ERROR_TCP_SOCKET_READ_FAILED        1068
#define ERROR_TCP_SOCKET_COPY_FAILED        1069
#define ERROR_TCP_SOCKET_WRITE_FAILED       1070
#define ERROR_TCP_SOCKET_CONNECT            1071

/*************************************************************/
#define ERROR_CONFIG_INVALID                1072
#define ERROR_CONFIG_DIRECTIVE              1073
#define ERROR_CONFIG_BLOCK_START            1074
#define ERROR_CONFIG_BLOCK_END              1075
#define ERROR_CONFIG_EOF                    1076
#define ERROR_CONFIG_LOAD_FILE              1077
#define ERROR_PROXY_PASS                    1078
#define ERROR_SERVER_CONFIG_EMPTY           1079

/************************* http ****************************/
#define ERROR_HTTP_HEADER_PARSER            1080
#define ERROR_HTTP_INVALID_CHUNK_HEADER     1081

#define ERROR_FLV_READ_HEADER               1082
#define ERROR_FLV_HEADER_TYPE               1083
#define ERROR_FLV_READ_TAG_HEADER           1084
#define ERROR_FLV_READ_TAG_BODY             1085
#define ERROR_FLV_READ_PREVIOUS_TAG_SIZE    1086

#define ERROR_OPEN_FILE                     1087
#define ERROR_FILE_NOT_EXIST                1088
#define ERROR_SEND_FILE                     1089
#define ERROR_CREATE_DIR                    1090
#define ERROR_RENAME_FILE                   1091
#define ERROR_WRITE_FILE                    1092

#define ERROR_REFERER_VERIFY                1093

#define ERROR_HTTP_PARSE_URI                1094
#define ERROR_HTTP_READ_BODY                1095
#define ERROR_RESPONSE_CODE                 1096

#define ERROR_HTTP_WRITE_VERIFY_VALUE       1097
#define ERROR_HTTP_VERIFY_STATUS_CODE       1098
#define ERROR_HTTP_VERIFY_NO_CONTENT_LENGTH 1099

#define ERROR_HTTP_SENDFILE_WRITE_HEADER    1100
#define ERROR_HTTP_STREAM_LIVE_WRITE_HEADER 1101
#define ERROR_HTTP_STREAM_RECV_WRITE_HEADER 1102
#define ERROR_HTTP_CONN_WRITE_REFUSE        1103

#define ERROR_PLAY_EDGE_START_RTMP          1104
#define ERROR_PLAY_EDGE_START_FLV           1105

#define ERROR_HTTP_WRITE_FLV_PLAY_HEADER       1106
#define ERROR_HTTP_WRITE_FLV_PUBLISH_HEADER    1107

#define ERROR_HTTP_READ_HEADER              1108
#define ERROR_HTTP_COPY_HEADER              1109

#define ERROR_HTTP_WRITE_FLV_TAG            1110
#define ERROR_HTTP_WRITE_TS_DATA            1111

#define ERROR_HTTP_INVALID_METHOD           1112
#define ERROR_HTTP_REQUEST_UNSUPPORTED      1113
#define ERROR_HTTP_GENERATE_REQUEST         1114
#define ERROR_HTTP_FLV_LIVE_REJECT          1115
#define ERROR_HTTP_FLV_RECV_REJECT          1116
#define ERROR_HTTP_FLV_PULL_REJECTED        1117
#define ERROR_HTTP_SEND_FILE_REJECT         1118

#define ERROR_HTTP_SEND_RESPONSE_HEADER     1119
#define ERROR_HTTP_SEND_REQUEST_HEADER      1120

#define ERROR_HTTP_CHUNKED_EOF              1121

#define ERROR_HTTP_TS_LIVE_REJECT           1122
#define ERROR_HTTP_TS_RECV_REJECT           1123
#define ERROR_HTTP_TS_PULL_REJECTED         1124

#define ERROR_TS_READ_BODY                  1125

#define ERROR_TS_MUXER_INIT_VIDEO           1126
#define ERROR_TS_MUXER_INIT_AUDIO           1127
#define ERROR_TS_MUXER_VIDEO_MUXER          1128
#define ERROR_TS_MUXER_AUDIO_MUXER          1129

#define ERROR_TS_DEMUXER                    1130

/************************* server ****************************/
#define ERROR_GENERATE_REQUEST              1131
#define ERROR_GET_CONFIG_VALUE              1132
#define ERROR_EGDE_GET_IP_PORT              1133
#define ERROR_SERVER_PROXY_MESSAGE          1134

#define ERROR_SOURCE_ONPLAY                 1135
#define ERROR_SOURCE_ONPUBLISH              1136
#define ERROR_SOURCE_ADD_CONNECTION         1137
#define ERROR_SOURCE_ADD_RELOAD             1138

#define ERROR_NO_HANDLER                    1139

#define ERROR_LOOKUP_HOST                   1140


#endif // KERNEL_ERRNO_HPP
