#include "blockchain.h"

/**
 * Creating a new list
 * 
 * @return New allocated list
 */
list* list_create()
{
    list* new_list = malloc(sizeof(list));

    if(new_list == NULL)
        return NULL;

    new_list->head = NULL;
    new_list->length = 0;
    return new_list;
}

/**
 * Adding block to the blockchain
 * 
 * @param prev Previous block to link
 * @param data Data of current block
 * @param length Length of current block
 * @return New block
 */
block_t* create_new_block(const block_t* prev, const char* data, uint32_t length){
    block_t* new_block = malloc(sizeof(block_t));
    //new_block->header = malloc(sizeof(block_header_t));

    new_block->header.data_length = length;
    
    new_block->header.timestamp = (uint32_t)time(NULL); 
    new_block->header.nounce = (uint32_t)0;

    if(prev!=NULL){
        hash256((const char *)prev->body,new_block->header.previous_hash);
    }
    else{
        //Genesis block setting
        memset(new_block->header.previous_hash, 0, sizeof(new_block->header.previous_hash));
    }
    //Hashing the data of current block
    hash256(data,new_block->header.data_hash);
    
    //Data to body
    new_block->body = (void*) data;

    return new_block;
}
