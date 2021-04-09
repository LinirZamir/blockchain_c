#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h> 

#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/sha.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>

#include "data_containers/dict.h"
#include "data_containers/linked_list.h"
#include "data_containers/log.h"

#include "nanomsg/include/nn.h"
#include "nanomsg/include/pipeline.h"

//Default sizes
#define DEFAULT_STR_SIZE 1024
#define ADDRESS_SIZE 64
#define TIMEOUT 50
#define MESSAGE_LENGTH 100000

//Error definition
#define ERR_FILE 2
#define ERR_LENGTH 3
#define ERR_SOCKET 4
#define ERR_NULL 5
#define ERR_GENERAL 6

////////////////////////////////////////////////////////////
//Socket struct
typedef struct socket_item {
    int socket;
    unsigned int last_used;
} socket_item;

typedef struct raft_node {
    char ip_address[64];
    char public_key[64];
    socket_item* socket_it;
    int leader; //0 - no, 1 - yes
}raft_node;


//Message item structure
typedef struct message_item {
    char toWhom[300];
    char message[30000];
    unsigned int tries;
} message_item;



//messages.c
int announce_existance(bt_node* in_dict, void* data);

//utils.c
int add_node_to_dict(char* server_address, dict* chain_nodes);
int read_nodes_from_file(const char* filename, dict* chain_nodes);
int create_raft_socket(const char* input, socket_item** sock_item);
int setup_message(message_item* in_message);
void shutdown(int dummy);
void* send_message(list* in_list, li_node* input, void* data);
void* process_inbound(list* in_list, li_node* input, void* data);

//sockets.c
void* in_server();
void* out_server();
void* inbound_executor();

//messages.process.c
int register_new_node(char* input);
void process_message(const char* in_msg, int msg_len);

//ISRaft.c