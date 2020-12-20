#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>

#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/sha.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>

#include "data_containers/dict.h"

#include "nanomsg/include/nn.h"
#include "nanomsg/include/pipeline.h"

#define ERR_NULL 2
#define ERR_FILE 3

#define BLOCK_STR_SIZE 30000

typedef struct block_header_t{
    uint32_t data_length;
    uint32_t timestamp;
    uint32_t nounce;

    unsigned char data_hash[32];
    unsigned char previous_hash[32];  
}block_header_t;

typedef struct block_t{
    void * body; 
    int block_no;
    block_header_t header;
}block_t;

//blockchain structure
typedef struct blockchain {
    block_t* head;
    int length;
} blockchain;

//utils.c
blockchain* new_chain();
block_t* create_new_block(const block_t* prev, const char* data, uint32_t length);
int read_chain_from_file(blockchain* in_chain, const char* filename);
int read_nodes_from_file(const char* filename, dict* dict_nodes);

//Blockchain.c
int hash256(const char *input_data, unsigned char *output_data);
