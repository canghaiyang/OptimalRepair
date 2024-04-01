#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <netdb.h>
#include <ifaddrs.h>

#include "ec_config.h"
#include "share_func.h"
#include "file_opt.h"
#include "network_transfer.h"
#include "bandwidth.h"
#include "repair.h"

int read_k_chunks_storagenodes(char **data, char *dst_filename, int *sockfd_array, int ok_node_list[])
{
    int i, j;
    metadata_t *metadata_array = (metadata_t *)malloc(sizeof(metadata_t) * EC_N); // only the func free
    pthread_t tid[EC_N];

    /* Recv to k storages that saved data chunk */
    for (i = 0; i < EC_K; i++)
    {
        j = ok_node_list[i];
        metadata_array[i].sockfd = sockfd_array[j];
        metadata_array[i].opt_type = 1;
        metadata_array[i].data = data[i];
        memset(metadata_array[i].dst_filename_storagenodes, 0, sizeof(metadata_array[i].dst_filename_storagenodes));
        sprintf(metadata_array[i].dst_filename_storagenodes, "%s_%d", dst_filename, j + 1); // filename on storages

        /* Create thread to send one chunk to one storages */
        if (pthread_create(&tid[i], NULL, recv_metadata_chunk, (void *)&metadata_array[i]) != 0)
            ERROR_RETURN_VALUE("Fail create thread");
    }
    for (i = 0; i < EC_K; i++)
        if (pthread_join(tid[i], NULL) != 0)
            ERROR_RETURN_VALUE("Fail join thread");
    free(metadata_array);
    return EC_OK;
}

int write_chunks_storagenodes(char **data, char **coding, char *dst_filename, int *sockfd_array)
{
    int i; // loop control variables

    metadata_t *metadata_array = (metadata_t *)malloc(sizeof(metadata_t) * EC_N); // only the func free
    pthread_t tid[EC_N];

    /* Send to each storagenodes */
    for (i = 0; i < EC_N; i++)
    {
        metadata_array[i].sockfd = sockfd_array[i];
        metadata_array[i].opt_type = 0;
        if (i < EC_K)
            metadata_array[i].data = data[i];
        else
            metadata_array[i].data = coding[i - EC_K];
        memset(metadata_array[i].dst_filename_storagenodes, 0, sizeof(metadata_array[i].dst_filename_storagenodes));
        sprintf(metadata_array[i].dst_filename_storagenodes, "%s_%d", dst_filename, i + 1); // filename on storagenodes

        /* Create thread to send one chunk to one storagenodes */
        if (pthread_create(&tid[i], NULL, send_metadata_chunk, (void *)&metadata_array[i]) != 0)
            ERROR_RETURN_VALUE("Fail create thread");
    }

    for (i = 0; i < EC_N; i++)
        if (pthread_join(tid[i], NULL) != 0)
            ERROR_RETURN_VALUE("Fail join thread");
    free(metadata_array);
    return EC_OK;
}

int request_tra_repair(int storagenodes_fail, char *dst_filename_healthy, char *dst_filename_failure, int *sockfd_array)
{
    int i; // loop control variables

    metadata_t *metadata_array = (metadata_t *)malloc(sizeof(metadata_t) * EC_N); // only the func free
    pthread_t tid[EC_N];

    /* Send to each storagenodes */
    for (i = 0; i < EC_K + 1; i++)
    {
        metadata_array[i].sockfd = sockfd_array[i];
        memset(metadata_array[i].dst_filename_storagenodes, 0, sizeof(metadata_array[i].dst_filename_storagenodes));
        if (i == storagenodes_fail)
        {
            metadata_array[i].opt_type = 2;
            sprintf(metadata_array[i].dst_filename_storagenodes, "%s_%d", dst_filename_failure, i + 1); // filename on storagenodes
        }
        else
        {
            metadata_array[i].opt_type = 3;
            sprintf(metadata_array[i].dst_filename_storagenodes, "%s_%d", dst_filename_healthy, i + 1); // filename on storagenodes
        }

        /* Create thread to send one request to one storagenodes */
        if (pthread_create(&tid[i], NULL, send_metadata, (void *)&metadata_array[i]) != 0)
            ERROR_RETURN_VALUE("Fail create thread");
    }

    for (i = 0; i < EC_K + 1; i++)
        if (pthread_join(tid[i], NULL) != 0)
            ERROR_RETURN_VALUE("Fail join thread");
    free(metadata_array);

    return EC_OK;
}

static int ec_read(int argc, char **argv)
{
    printf("[ec_read begin]\n");
    int i, j; // loop control variables

    /* File arguments */
    FILE *src_fp;  // src_file pointers
    int file_size; // size of file
    char src_filename[MAX_PATH_LEN] = {0}, dst_filename[MAX_PATH_LEN] = {0}, file_size_filename[MAX_PATH_LEN] = {0};
    char cur_directory[MAX_PATH_LEN] = {0}, ec_directory[MAX_PATH_LEN] = {0};
    getcwd(cur_directory, sizeof(cur_directory));
    strncpy(ec_directory, cur_directory, strlen(cur_directory) - strlen(SCRIPT_PATH)); // -6 to sub script/
    sprintf(src_filename, "%s%s%s", ec_directory, READ_PATH, argv[2]);                 // get src_filename
    sprintf(dst_filename, "%s%s%s", ec_directory, WRITE_PATH, argv[3]);                // get dst_filename
    sprintf(file_size_filename, "%s%s%s", ec_directory, FILE_SIZE_PATH, argv[3]);      // get file_size_filename to save file size of src file
    // printf("src_filename = %s, dst_filename = %s, file_size_filename = %s\n", src_filename, dst_filename, file_size_filename);

    /* EC arguments */
    int buffer_size = EC_K * CHUNK_SIZE;                       // stripe size
    char *buffer = (char *)malloc(sizeof(char) * buffer_size); // buffer for EC stripe
    if (buffer == NULL)
        ERROR_RETURN_VALUE("Fail malloc data buffer");

    /* ISA-L arguments */
    int *matrix = NULL;                                          // coding matrix
    char **data = (char **)malloc(sizeof(char *) * EC_K);        // data chunk
    char **coding = (char **)malloc(sizeof(char *) * EC_M);      // coding chunk
    char **repair_data = (char **)malloc(sizeof(char *) * EC_K); // repair data chunk

    /* initialize data buffer and coding buffer */
    for (i = 0; i < EC_M; i++)
    {
        coding[i] = (char *)malloc(sizeof(char) * CHUNK_SIZE);
        if (coding[i] == NULL)
            ERROR_RETURN_VALUE("Fail malloc coding buffer");
    }
    for (i = 0; i < EC_K; i++)
        data[i] = buffer + (i * CHUNK_SIZE);

    /* check file_size */
    char file_size_buffer[MAX_PATH_LEN] = {0};
    if (open_read_file(file_size_filename, "rb", file_size_buffer, sizeof(file_size_buffer)) == EC_ERROR)
        ERROR_RETURN_VALUE("open_read_file");
    if ((file_size = atoi(file_size_buffer)) != buffer_size) // check readsize
        ERROR_RETURN_VALUE("error readsize: different chunksize or ec_k+ec_m");

    /* repair arguments */
    int err_node_list[EC_N]; // location of nodes_error
    int ok_node_list[EC_N];  // location of nodes_ok
    int nerrs = 0;           // number of nodes_error
    int nok = 0;             // number of nodes_ok

    /* initialize_network and get number&location of nodes_error and nodes_ok */
    /* read chunk from k storagenodes */
    int sockfd_array[EC_N];
    for (i = 0; i < EC_N; i++)
    {
        sockfd_array[i] = -1;
        if ((initialize_network(&sockfd_array[i], EC_CLIENT_STORAGENODES_PORT, i)) == EC_ERROR) // only the func close socket
            err_node_list[nerrs++] = i;
        else
            ok_node_list[nok++] = i;
    }
    if (nok < EC_K)
        ERROR_RETURN_VALUE("error num_nodes more than EC_M");
    if ((read_k_chunks_storagenodes(data, dst_filename, sockfd_array, ok_node_list)) == EC_ERROR)
        ERROR_RETURN_VALUE("Fail recv k chunks");
    for (i = EC_K; i < nok; i++)
        shutdown(sockfd_array[ok_node_list[i]], SHUT_RDWR);

    /* repair data chunks */
    if (repair_data_chunks(ok_node_list, nok, err_node_list, nerrs, data, coding, repair_data) == EC_ERROR)
        ERROR_RETURN_VALUE("repair_data_chunks");

    /* clear file and write data */
    if (open_write_file_mul(src_filename, "wb", repair_data, CHUNK_SIZE, EC_K) == EC_ERROR)
        ERROR_RETURN_VALUE("open_write_file_mul");

    for (i = 0; i < EC_M; i++)
        free(coding[i]);
    free(repair_data);
    free(coding);
    free(data);
    free(buffer);

    printf("[ec_read end]\n");
    return EC_OK;
}

static int ec_write(int argc, char **argv)
{
    printf("[ec_write begin]\n");

    int i; // loop control variables

    /* File arguments */
    FILE *src_fp;            // src_file pointers
    int file_size;           // size of file
    struct stat file_status; // finding file size
    char src_filename[MAX_PATH_LEN] = {0}, dst_filename[MAX_PATH_LEN] = {0}, file_size_filename[MAX_PATH_LEN] = {0};
    char cur_directory[MAX_PATH_LEN] = {0}, ec_directory[MAX_PATH_LEN] = {0};
    getcwd(cur_directory, sizeof(cur_directory));
    strncpy(ec_directory, cur_directory, strlen(cur_directory) - strlen(SCRIPT_PATH)); // -6 to sub script/
    sprintf(src_filename, "%s%s%s", ec_directory, WRITE_PATH, argv[2]);                // get src_filename
    sprintf(dst_filename, "%s%s%s", ec_directory, WRITE_PATH, argv[3]);                // get dst_filename
    sprintf(file_size_filename, "%s%s%s", ec_directory, FILE_SIZE_PATH, argv[3]);      // get file_size_filename to save file size of src file
    // printf("src_filename = %s, dst_filename = %s, file_size_filename = %s\n", src_filename, dst_filename, file_size_filename);

    /* EC arguments */
    int buffer_size = EC_K * CHUNK_SIZE;                       // stripe size
    char *buffer = (char *)malloc(sizeof(char) * buffer_size); // buffer for EC stripe
    if (buffer == NULL)
        ERROR_RETURN_VALUE("Fail malloc data buffer");

    /* ISA-L arguments */
    int *matrix = NULL;                                     // coding matrix
    char **data = (char **)malloc(sizeof(char *) * EC_K);   // data chunk s
    char **coding = (char **)malloc(sizeof(char *) * EC_M); // coding chunks

    /* initialize data buffer and coding buffer */
    for (i = 0; i < EC_M; i++)
    {
        coding[i] = (char *)malloc(sizeof(char) * CHUNK_SIZE);
        if (coding[i] == NULL)
            ERROR_RETURN_VALUE("Fail malloc coding buffer");
    }
    for (i = 0; i < EC_K; i++)
        data[i] = buffer + (i * CHUNK_SIZE);

    /*One IO read file to buffer*/
    if ((file_size = get_size_file(src_filename)) != buffer_size)
        ERROR_RETURN_VALUE("file_size != EC_K * CHUNK_SIZE");
    if (read_file_to_buffer(src_filename, buffer, buffer_size) == EC_ERROR)
        ERROR_RETURN_VALUE("error read_file_to_buffer");

    /* Encode */
    encode_data_chunks(data, coding);

    /* write chunks to each storages */
    int sockfd_array[EC_N];
    for (i = 0; i < EC_N; i++)
        if ((initialize_network(&sockfd_array[i], EC_CLIENT_STORAGENODES_PORT, i)) == EC_ERROR) // only the func close socket
            ERROR_RETURN_VALUE("Fail initialize network");
    if ((write_chunks_storagenodes(data, coding, dst_filename, sockfd_array)) == EC_ERROR)
        ERROR_RETURN_VALUE("Fail send chunks to storagenodes");
    for (i = 0; i < EC_N; i++)
        shutdown(sockfd_array[i], SHUT_RDWR);

    /* write file size to file_size filename */
    char file_size_buffer[MAX_PATH_LEN] = {0};
    sprintf(file_size_buffer, "%d", file_size);
    if (open_write_file(file_size_filename, "wb", file_size_buffer, sizeof(file_size_buffer)) == EC_ERROR)
        ERROR_RETURN_VALUE("open_write_file");

    for (i = 0; i < EC_M; i++)
        free(coding[i]);
    free(coding);
    free(data);
    free(buffer);

    printf("[ec_write end]\n");
    return EC_OK;
}

static int ec_test_tra_repair(int argc, char **argv)
{
    printf("[ec_test_tra_repair begin]\n");
    int i; // loop control variables

    /* File arguments */
    FILE *src_fp;  // src_file pointers
    int file_size; // size of file
    char dst_filename_failure[MAX_PATH_LEN] = {0}, dst_filename_healthy[MAX_PATH_LEN] = {0};
    char cur_directory[MAX_PATH_LEN] = {0}, ec_directory[MAX_PATH_LEN] = {0};
    getcwd(cur_directory, sizeof(cur_directory));
    strncpy(ec_directory, cur_directory, strlen(cur_directory) - strlen(SCRIPT_PATH)); // -6 to sub script/
    sprintf(dst_filename_failure, "%s%s%s", ec_directory, REPAIR_PATH, argv[2]);       // get dst_filename_failure
    sprintf(dst_filename_healthy, "%s%s%s", ec_directory, WRITE_PATH, argv[3]);        // get dst_filename_healthy
    // printf("dst_filename_healthy = %s, dst_filename_failure = %s\n", dst_filename_healthy, dst_filename_failure);

    int storagenodes_fail = STORAGENODE_FAIL; // 1 failure storagenodes

    /* request each storagenodes for 1 failure storagenodes and k healthy storagenodes */
    int sockfd_array[EC_N];
    for (i = 0; i < EC_K + 1; i++)
        if ((initialize_network(&sockfd_array[i], EC_CLIENT_STORAGENODES_PORT, i)) == EC_ERROR) // only the func close socket
            ERROR_RETURN_VALUE("Fail initialize network");
    if ((request_tra_repair(storagenodes_fail, dst_filename_healthy, dst_filename_failure, sockfd_array)) == EC_ERROR)
        ERROR_RETURN_VALUE("Fail send request_tra_repair to storagenodes");
    for (i = 0; i < EC_K + 1; i++)
        shutdown(sockfd_array[i], SHUT_RDWR);

    printf("[ec_test_tra_repair end]\n");
    return EC_OK;
}
static int ec_test_piv_repair(int argc, char **argv)
{
    printf("[ec_test_piv_repair begin]\n");

    printf("[ec_test_piv_repair end]\n");
    return EC_OK;
}
static int ec_test_ppt_repair(int argc, char **argv)
{
    printf("[ec_test_ppt_repair begin]\n");

    printf("[ec_test_ppt_repair end]\n");
    return EC_OK;
}
static int ec_test_rp_repair(int argc, char **argv)
{
    printf("[ec_test_rp_repair begin]\n");

    int i; // loop control variables

    int uplink_bandwidth[EC_N_MAX], downlink_bandwidth[EC_N_MAX], light_heavy_flag = 1;
    int **nodes_bandwidth = (int **)malloc(EC_N * sizeof(int *));
    int *nodes_bandwidth_temp = (int *)malloc(EC_N * EC_N * sizeof(int));
    for (i = 0; i < EC_N; i++)
        nodes_bandwidth[i] = nodes_bandwidth_temp + (i * EC_N);

    if (read_updown_bandwidth(uplink_bandwidth, downlink_bandwidth, light_heavy_flag, BAND_LOCATION) == EC_ERROR)
        ERROR_RETURN_VALUE("Fail read_updown_bandwidth");
    get_bandwidth_between_nodes(uplink_bandwidth, downlink_bandwidth, nodes_bandwidth);

    /*
    1. 确定故障节点（也是最后的接受节点，带宽为上限）
    2. 带宽确定：带宽来源，上下行链接带宽和节点间带宽转换
    3. 算法准备：
    4. 算法执行：
    */
    free(nodes_bandwidth_temp);
    free(nodes_bandwidth);

    printf("[ec_test_ppt_repair end]\n");
    return EC_OK;
}
static int ec_test_new_repair(int argc, char **argv)
{
    printf("[ec_test_new_repair begin]\n");

    printf("[ec_test_new_repair end]\n");
    return EC_OK;
}

static void help(int argc, char **argv)
{
    fprintf(stderr, "Usage: %s cmd...\n"
                    "\t -h \n"
                    "\t -w <src_filename> <dst_filename> \n"
                    "\t -r <src_filename> <dst_filename> \n"
                    "\t -kw <src_filename> <dst_filename> \n"
                    "\t  \n"
                    "\t Example1: -w src_10MB.mp4 dst_10MB.mp4 \n"
                    "\t <src_filename> saved on ec_test/test_file/write/ for client\n"
                    "\t <dst_filename> saved on ec_test/test_file/write/ for storages \n"
                    "\t Example2: -r read_10MB.mp4 dst_10MB.mp4  \n"
                    "\t <src_filename> saved on ec_test/test_file/read/ for storages\n"
                    "\t <dst_filename> saved on ec_test/test_file/write/ for client \n"
                    "\t Tip: saved filename on storages actually is dst_filenameX_Y, \n"
                    "\t the X is Xth stripe and the Y is Yth chunk.\n"
                    "\t  \n",
            argv[0]);
}

/**
 * argv[1] cmd
 * argv[2] src_filename
 * argv[3] dst_filename
 */

int main(int argc, char *argv[]) // cur_dir is script
{
    printf("[ec_client begin]\n");

    if (argc < 2)
    {
        help(argc, argv);
        return EC_OK;
    }
    char cmd[16] = {0};
    strcpy(cmd, argv[1]);

    if (0 == strncmp(cmd, "-h", strlen("-h"))) // cmd -h: help information
    {
        help(argc, argv);
        return EC_OK;
    }
    else if (0 == strncmp(cmd, "-w", strlen("-w"))) // cmd -w: erasure coding write
    {
        if (ec_write(argc, argv) == EC_ERROR)
            printf("Fail ec_write\n");
    }
    else if (0 == strncmp(cmd, "-r", strlen("-r"))) // cmd -r: erasure coding read
    {
        if (ec_read(argc, argv) == EC_ERROR)
            printf("Fail ec_read\n");
    }
    else if (0 == strncmp(cmd, "-trtra", strlen("-trtra"))) // cmd -trtra: erasure coding repair test: traditional
    {
        if (ec_test_tra_repair(argc, argv) == EC_ERROR)
            printf("Fail ec_test_tra_repair\n");
    }
    else if (0 == strncmp(cmd, "-trpiv", strlen("-trpiv"))) // cmd -trpiv: erasure coding repair test: pivot
    {
        if (ec_test_tra_repair(argc, argv) == EC_ERROR)
            printf("Fail ec_test_tra_repair\n");
    }
    else if (0 == strncmp(cmd, "-trppt", strlen("-trppt"))) // cmd -trppt: erasure coding repair test: ppt
    {
        if (ec_test_tra_repair(argc, argv) == EC_ERROR)
            printf("Fail ec_test_tra_repair\n");
    }
    else if (0 == strncmp(cmd, "-trrp", strlen("-trrp"))) // cmd -trrp: eerasure coding repair test: repair pipeline
    {
        if (ec_test_tra_repair(argc, argv) == EC_ERROR)
            printf("Fail ec_test_tra_repair\n");
    }
    else if (0 == strncmp(cmd, "-trnew", strlen("-trnew"))) // cmd -trnew: erasure coding repair test: new method
    {
        if (ec_test_tra_repair(argc, argv) == EC_ERROR)
            printf("Fail ec_test_tra_repair\n");
    }
    else
    {
        printf("Error operation, please read help\n");
        help(argc, argv);
    }

    return EC_OK;
}

// struct timespec time_start, time_end; // test time
// long long elapsed_time;
// clock_gettime(CLOCK_MONOTONIC, &time_start);
// clock_gettime(CLOCK_MONOTONIC, &time_end);                                                            // Get the start time
// elapsed_time = (time_end.tv_sec - time_start.tv_sec) * 1e9 + (time_end.tv_nsec - time_start.tv_nsec); // Calculate execution time (in milliseconds)
// printf("ychtime =  %lld us\n", elapsed_time / 1000);                                                  // Print execution time