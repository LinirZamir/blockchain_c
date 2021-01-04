#include "ISRaft.h"

extern dict* out_sockets;
extern char our_ip[300];
extern list* outbound_msg_queue;

/**
 * Creating a new chain list
 * 
 * @return New allocated list
 */
datalog* new_chain()
{
    datalog* bc = malloc(sizeof(datalog));
    
    bc->head = create_new_block(NULL, "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks", 69);
    bc->length = 1;

    return bc;
}


void block_t_discard_list(block_t* head)
{   
    if(head == NULL) return;
    //Single element sized list

    if(head->next == NULL)
    {
        free(head);
        return;
    }

    block_t* temp = head;
    temp = temp->next;

    while(head != NULL)
    {
        free(head);
        head = temp;
        if(temp != NULL)
            temp = temp->next;
    }
    head = NULL;
}

//Destroy the chain and free every link in it
int discard_chain(datalog* in_chain) {

    if(in_chain == NULL) return 0;

    //Discard list of blocks in chain
    block_t_discard_list(in_chain->head);

    //Free memory in struct
    free(in_chain);

    return 1;
}


/**
 * Adding block to the datalog
 * 
 * @param prev Previous block to link
 * @param data Data of current block
 * @param length Length of current block
 * @return New block
 */
block_t* create_new_block(block_t* prev, const char* data, uint32_t length){
    block_t* new_block = malloc(sizeof(block_t));

    //new_block->header = malloc(sizeof(block_header_t));

    new_block->header.data_length = length;
    
    new_block->header.timestamp = (uint32_t)time(NULL); 
    new_block->header.nounce = (uint32_t)0;

    if(prev!=NULL){
        hash256((const char *)prev->body,new_block->header.previous_hash);
        prev->next = new_block;
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

int read_chain_from_file(datalog* in_chain, const char* filename){
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

//Create a socket to be used for the given address
int create_socket(const char* input) {

    if(strlen(input) > 299) return 0;

    char address[SHORT_MESSAGE_LENGTH];
    strcpy(address, input);
    
    printf("Creating socket for... %s\n", address);

    socket_item new_out_socket;

    new_out_socket.last_used = time(NULL);

    new_out_socket.socket = nn_socket(AF_SP, NN_PUSH);
    if(new_out_socket.socket < 0) return 0;
    int timeout = 100;
    if(nn_setsockopt(new_out_socket.socket, NN_SOL_SOCKET, NN_SNDTIMEO, &timeout, sizeof(timeout)) < 0) return 0;

    if(nn_connect (new_out_socket.socket, address) < 0){
        printf("Connection Error.\n");
        nn_close(new_out_socket.socket);
    }
    
    dict_insert(out_sockets,address,&new_out_socket,sizeof(new_out_socket));

    return 1;
}

//Empty message struct
void setup_message(message_item* in_message) {
    
    if(in_message == NULL) return;

    in_message->tries = 0;
    memset(in_message->toWhom, 0, sizeof(in_message->toWhom));
    return;
}

//Read the nodes on server, and writes a new self address to file
int read_nodes_from_file(const char* filename, dict* dict_nodes){
    ///File has to be in ipc:///tmp/pipeline_0.ipc format
    printf("Reading nodes from file: '%s'\n", filename);
    FILE* chain_file = fopen(filename, "ab+");
    if(chain_file == NULL) return ERR_FILE;
    
    char buff[BLOCK_STR_SIZE] = {0};
    int curr_index = 0;
    int counter = 0;
    while(fgets(buff, sizeof(buff), chain_file)){
        if(buff[strlen(buff)-1] == '\n') buff[strlen(buff)-1] = 0; //Check this line
        printf("Reading from file: %s\n", buff);
        curr_index = atoi(buff+strlen(buff)-5);
        dict_insert(dict_nodes,buff,"datainside",strlen("datainside"));
        create_socket(buff);
        counter++;
    }
    //handle empty file
    if(buff[0] == 0){
        fprintf(chain_file, "ipc:///tmp/pipeline_0.ipc\n");
        fclose (chain_file);
        return 0;
    }else{
        fprintf(chain_file, "ipc:///tmp/pipeline_%d.ipc\n",curr_index+1);
        fclose (chain_file);
        return curr_index+1;
    }
    fclose (chain_file);
    return ERR_FILE;
}

int announce_existance(bt_node* in_dict, void* data){
    if(in_dict == NULL || in_dict->size > 300) return ERR_NULL;

    message_item announcement;
    setup_message(&announcement);
    strcpy(announcement.toWhom,in_dict->key);
    strcpy(announcement.message, "N ");
    strcat(announcement.message, our_ip);

    li_append(outbound_msg_queue,&announcement,sizeof(announcement));
}

int AppendEntries(bt_node* in_dict, void* data){
    if(in_dict == NULL || in_dict->size > 300) return ERR_NULL;

    message_item announcement;
    setup_message(&announcement);
    strcpy(announcement.toWhom,in_dict->key);
    strcpy(announcement.message, "A ");
    strcat(announcement.message, our_ip);

    li_append(outbound_msg_queue,&announcement,sizeof(announcement));
}


int announce_exit(bt_node* in_dict, void* data){
    if(in_dict == NULL || in_dict->size > 300) return ERR_NULL;

    message_item announcement;
    setup_message(&announcement);
    strcpy(announcement.toWhom,in_dict->key);
    strcpy(announcement.message, "D ");
    strcat(announcement.message, our_ip);

    li_append(outbound_msg_queue,&announcement,sizeof(announcement));
}

int announce_mined(bt_node* in_dict, void* data){
    if(in_dict == NULL || in_dict->size > 300) return ERR_NULL;

    message_item announcement;
    setup_message(&announcement);
    strcpy(announcement.toWhom,in_dict->key);
    strcpy(announcement.message, "B ");
    strcat(announcement.message, our_ip);

    li_append(outbound_msg_queue,&announcement,sizeof(announcement));
}
