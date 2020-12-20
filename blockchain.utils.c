#include "blockchain.h"

/**
 * Creating a new chain list
 * 
 * @return New allocated list
 */
blockchain* new_chain()
{
    blockchain* bc = malloc(sizeof(blockchain));
    
    bc->head = create_new_block(NULL, "GENESIS", strlen("GENESIS"));
    bc->length = 1;

    return bc;
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

int read_chain_from_file(blockchain* in_chain, const char* filename){
    //TODO fix read_chain func
    printf("Reading chain from file: '%s'\n", filename);
    FILE* chain_file = fopen(filename, "r");
    if(chain_file == NULL) return ERR_FILE;
    
    char buff[BLOCK_STR_SIZE] = {0};
    int curr_index = 0;
    while(fgets(buff, sizeof(buff), chain_file)){
        if(buff[strlen(buff)-1] == '\n') buff[strlen(buff)-1] = 0; //Check this line
        printf("Reading from file: %s\n", buff);
    }

    return ERR_FILE;
}

int read_nodes_from_file(const char* filename, dict* dict_nodes){
    ///File has to be in ipc:///tmp/pipeline_0.ipc format
    printf("Reading nodes from file: '%s'\n", filename);
    FILE* chain_file = fopen(filename, "r+");
    if(chain_file == NULL) return ERR_FILE;
    
    char buff[BLOCK_STR_SIZE] = {0};
    int curr_index = 0;
    while(fgets(buff, sizeof(buff), chain_file)){
        if(buff[strlen(buff)-1] == '\n') buff[strlen(buff)-1] = 0; //Check this line
        printf("Reading from file: %s\n", buff);
        dict_insert(dict_nodes,buff,"datainside",strlen("datainside"));
    }
    if(buff[0] == 0){
        fprintf(chain_file, "ipc:///tmp/pipeline_0.ipc\n");
        fclose (chain_file);
        return 0;
    }else{
        fprintf(chain_file, "ipc:///tmp/pipeline_%d.ipc\n",atoi(buff+strlen(buff)-5)+1);
        fclose (chain_file);
        return atoi(buff+strlen(buff)-5)+1;
    }
    fclose (chain_file);
    return ERR_FILE;
}
