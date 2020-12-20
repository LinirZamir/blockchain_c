#include "blockchain.h"

//This nodes IP
char our_ip[300] = {0};

//Blockchain
blockchain* l_chain;
dict* chain_nodes;

//identifications
char chain_filename[300];
uint8_t target[32]; //target difficulty


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
    
    //load defaults
    strcpy(chain_filename, "chain_0.israft");

    //Set PoW algorithm difficulty (0xFF)
    memset(target, 0, sizeof(target));
    target[2] = 0xFF;
    
    //Create our blockchain and Process chain file
    int chain_good = read_chain_from_file(l_chain, chain_filename);
    if(chain_good!=0) {
        l_chain = new_chain(); //TODO Write blockchain to file
    }
   
   //Initialization of nodes on the server
    chain_nodes = dict_create();
    int loc = read_nodes_from_file("nodes.conf", chain_nodes);
    sprintf(our_ip, "ipc:///tmp/pipeline_%d.ipc",loc);

    

    //TODO Init connection
    //TODO initiate nanomsg
    //TODO ping when a new node joins

    proof_of_work(l_chain->head);
    
    return 0;
}
