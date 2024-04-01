#ifndef EC_CONFIG_H
#define EC_CONFIG_H


#define LOG 0

#define EC_K 2                   // k of k+m EC
#define EC_M 2                   // m of k+m EC, larger than 1
#define EC_N 4                   // n of k+m EC
#define EC_W 8                   // finite field 2^w
#define CHUNK_SIZE (1 * 1048576) // unit Byte
#define EC_X 3                   // x number of encoded nodes

#define SEND_DATANODE 1 // 1, send chunks to storages; 0, just locally encode
#define SEND_METHOD 0   // 1, send in serial; 0,send in parallel
#define RECV_METHOD 0   // 1, recv in serial; 0,recv in parallel

/* repair */
#define STORAGENODE_FAIL 0 // Position relative to STORAGENODES_START_IP_ADDR: [0,K-1]

/* file */
#define SCRIPT_PATH "script"                            // saved executable file
#define BUILD_PATH "build"                              // saved executable file
#define WRITE_PATH "test_file/write/"                   // src_file and dst_file saved path
#define READ_PATH "test_file/read/"                     // src_file and dst_file saved path
#define FILE_SIZE_PATH "test_file/file_size/file_size_" // file_size file saved path
#define BANDWIDTH_PATH "test_file/bandwidth/"           // nodes bandwidth saved path
#define REPAIR_PATH "test_file/repair/"                 // repair data saved path
#define MAX_PATH_LEN 256                                // Max length of file path

/* network */
#define IP_PREFIX "192.168.7."           // storages ip prefix
#define STORAGENODES_START_IP_ADDR 102   // end_ip=start_ip+k+m, ip_addr_start (1-255,default ip_addr_end=ip_addr_start+k+m)
#define EC_CLIENT_STORAGENODES_PORT 8000 // EC network port between client and storagenodes
#define EC_STORAGENODES_PORT 8001        // EC network port between storagenodes and storagenodes
#define EC_PS_STORAGENODES_PORT 8002     // EC network port between programmable swtich and storagenodes
#define EC_PS_CLIENT_PORT 8003           // EC network port between programmable swtich and client

/* bandwidth */
#define NUM_BAND 5          // Number of heterogeneous network environments in bandwidth.cpp
#define EC_N_MAX 16         //  Maximum EC_N
#define MAX_LINE_LENGTH 100 // MAX line length in bandwidth file
#define BAND_LOCATION 1     // Test bandwidth is the BAND_LOCATION-th in the bandwidth file

/* other */
#define WAIT_S 1 // waiting time/second for ready

#define INT_MAX 2147483647

#define EC_ERROR -1
#define EC_OK 0

#define ERROR_RETURN_VALUE(message)                                           \
    do                                                                \
    {                                                                 \
        fprintf(stderr, "[%s:%d] %s\n", __FILE__, __LINE__, message); \
        perror("OS_errorcode_information");                           \
        fflush(stdout);                                               \
        fflush(stderr);                                               \
        return EC_ERROR;                                               \
    } while (0)

#define ERROR_RETURN_NULL(message)                                           \
    do                                                                \
    {                                                                 \
        fprintf(stderr, "[%s:%d] %s\n", __FILE__, __LINE__, message); \
        perror("OS_errorcode_information");                           \
        fflush(stdout);                                               \
        fflush(stderr);                                               \
        return NULL;                                               \
    } while (0)

#define PRINT_FLUSH     \
    do                  \
    {                   \
        fflush(stdout); \
        fflush(stderr); \
    } while (0)

typedef struct metadata_s // chunk metadata and data
{
    int sockfd;     // network socket fd
    int opt_type;   // 0=ec_write 1=ec_read 2=tra_repair_fail 3=tra_repair_healthy
    int block_size; // chunk size
    int remain_block_size;
    int cur_block;
    int cur_eck;
    char *data;                                   // chunk data or block data
    char dst_filename_storagenodes[MAX_PATH_LEN]; // dst filename on storages
} metadata_t;

typedef void (*type_handle_func)(metadata_t *, int);

#endif