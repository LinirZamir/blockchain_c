#include "blockchain.h"

//This nodes IP
char * our_ip;

//Blockchain
list* l_chain;

//Difficuly of algorithm
uint8_t target[32];


/**
 * SHA256 hashing for input_data
 * 
 * @param input_data The data to be hashed
 * @param output_data The data output
 * @return Error code
 */
int hash256(const char *input_data, unsigned char *output_data){
    int ret = 0;

    size_t len = strlen(input_data);
    unsigned char tmp [32];
    SHA256((const unsigned char*)input_data, len, tmp);
    memcpy(output_data, tmp,32);

    return ret; 
}

/**
 * Stringify the header
 * 
 * @param head The head of a block
 * @return Stringify header
 */
char* header_to_string(block_header_t head){
    char *output = malloc(sizeof(block_header_t));
    char *tmp = malloc(sizeof(head.data_length));  //favacodes

    sprintf( tmp, "%d", head.data_length);
    memcpy(output,tmp,strlen(tmp));
    output[strlen(tmp)] = '\0';

    sprintf( tmp, "%d", head.timestamp);
    strcat(output, tmp);
    sprintf( tmp, "%d", head.nounce);
    strcat(output, tmp);
    strcat(output, head.data_hash);
    strcat(output, head.previous_hash);

    return output;
}

int proof_of_work(block_t* block){
    int ret = 0;
    unsigned char *tmp; 
    char* header_string;
    
    tmp = malloc(32);
    
    for(int i= 0; i<UINT32_MAX; i++){
        block->header.nounce = (uint32_t)i;
        header_string = header_to_string(block->header);
        hash256(header_string,tmp);

        if(memcmp(tmp, target, sizeof(tmp))<0){
            printf("FOUND!\n");
            return ret; 
        }
    }
    return ret; 
}

int main(int argc, const char* argv[]) {
    int ret = 0;
    char* gen = "genesisnounce";
    
    l_chain = list_create();
    //Set PoW algorithm difficulty (0xFF)
    memset(target, 0, sizeof(target));
    target[2] = 0xFF;
    
    //Create genesis block
    block_t* genesis = create_new_block(NULL, gen, strlen(gen));

    //Init connection


    proof_of_work(genesis);


    //Load defaults
    //strcpy(our_ip, "ipc:///tmp/pipeline_0.ipc");

return 0;
}
