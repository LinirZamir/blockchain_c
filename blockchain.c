#include "blockchain.h"

//This nodes IP
char our_ip[300] = {0};

//Blockchain
blockchain* l_chain;
dict* chain_nodes;
list* outbound_msg_queue; //holds outbound message structs
list* inbound_msg_queue; //holds recieved char* messages to execute

//identifications
char chain_filename[300];
uint8_t target[32]; //target difficulty

//Socket 
int socket_in;
int last_ping;
dict* out_sockets;

//Threads
pthread_mutex_t our_mutex;
pthread_t inbound_network_thread;
pthread_t outbound_network_thread;
pthread_t inbound_executor_thread;
int close_threads;


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
            return 1; 
        }
    }
    return ret; 
}

void* in_server(){
    int timeout = 50;
    
    socket_in = nn_socket (AF_SP, NN_PULL);
    assert(socket_in >= 0);
    assert(nn_bind(socket_in, our_ip) >= 0);
    assert(nn_setsockopt(socket_in, NN_SOL_SOCKET, NN_RCVTIMEO, &timeout, sizeof(timeout))>=0);

    printf("Inbound Socket Ready!\n");

    char buf[MESSAGE_LENGTH];

    while(true) {
        int bytes = nn_recv(socket_in, buf, sizeof(buf), 0);
        
        pthread_mutex_lock(&our_mutex);
        if(bytes > 0) {
            buf[bytes] = '\0';
            printf("\nRecieved %d bytes: \"%s\"\n", bytes, buf);
            li_append(inbound_msg_queue,buf,bytes);
        }
        if(close_threads) {
                pthread_mutex_unlock(&our_mutex);            
                return 0;
        }
        pthread_mutex_unlock(&our_mutex);            

    }
    return 0;
}

void* send_message(list* in_list, li_node* input, void* data){
    int the_socket;
    int used_rare_socket = 0;

    if(input == NULL) return NULL;

    message_item* our_message = (message_item*)input->data;
    if(our_message == NULL) return NULL;
    socket_item* sock_out_to_use = (socket_item*)dict_access(out_sockets,our_message->toWhom);

    if(our_message->tries == 1) return NULL;

    the_socket = sock_out_to_use->socket;

    printf("Sending to: %s, ",our_message->toWhom);
    int bytes = nn_send (the_socket,  our_message->message, strlen(our_message->message), 0);
    printf("Bytes sent: %d\n", bytes);

    usleep(100);

    if(bytes > 0 || our_message->tries == 2) li_delete_node(in_list, input);
    else our_message->tries++;


    return NULL;


}

//Send out a ping to all other nodes every 30 seconds 
int ping_function() {

    if(time(NULL) - last_ping > 30) {
        printf("Pinging Nodes in List...\n");
        dict_foreach(chain_nodes,announce_existance, NULL);
        last_ping = time(NULL);
    }

    return 1;
}

//Outbound thread function - tries to send everything in outbound message queue
void* out_server() {

    while(true) {
        pthread_mutex_lock(&our_mutex);

        li_foreach(outbound_msg_queue, send_message, NULL);
        ping_function();

        if(close_threads) {
            pthread_mutex_unlock(&our_mutex);
            return NULL;
        }
        pthread_mutex_unlock(&our_mutex);

        usleep(100);
    }
}

//Remove node from dict
int remove_node(char* input){
    if(input == NULL) return ERR_NULL;
    printf("Removing Node: %s",input);

    //Remove from chain+socket dict
    dict_del_elem(chain_nodes,input,0);
    dict_del_elem(out_sockets, input, 0);

    return 0;

}

//Regster New Node and send out your chain length
int register_new_node(char* input) {

    if(input == NULL) return ERR_NULL;

    if(!strcmp(input, our_ip)) {
        printf("Someone is acting on the same IP\n");
        return 1;
    }

    if(strlen(input) > SHORT_MESSAGE_LENGTH) {
        printf("IP address too long to register!");
        return 0;
    }

    char add_who[SHORT_MESSAGE_LENGTH] = {0};
    strcpy(add_who,input);
    printf("Registering New Node...");

    if(dict_access(chain_nodes, input) != NULL){
        printf("Already registered. Updating time\n");

        socket_item* our_socket = (socket_item*)dict_access(out_sockets,add_who);
        if(our_socket != NULL) {
            our_socket->last_used = time(NULL);
        }
    }else{
        //Add to dict
        dict_insert(chain_nodes,input,"data",strlen("data"));
        //Create socket
        create_socket(input);

    }
}

//Free all outstanding memory
void shutdown(int dummy) {

    printf("\nCommencing shutdown!\n");

    close_threads = 1;

    //Send out remove existence
    dict_foreach(chain_nodes,announce_exit, NULL); //TODO Handle receive

    usleep(50);

    pthread_join(outbound_network_thread, NULL);
    pthread_join(inbound_network_thread, NULL);
    pthread_join(inbound_executor_thread, NULL);

    pthread_mutex_lock(&our_mutex);
    


    //Discard lists
    li_discard(outbound_msg_queue);
    li_discard(inbound_msg_queue);

    //Save blockchain to file

    //Discard blockchain
    discard_chain(l_chain);

    //Discard keys

    nn_close(socket_in);

    pthread_mutex_unlock(&our_mutex);

    pthread_mutex_destroy(&our_mutex);

    printf("Shutdown complete!\n");
    exit(0);
}


//Message type: T: transaction, P: post, B: block, N: new node, L: blockchain length, C: chain
void process_message(const char* in_msg) {
    if(in_msg == NULL) return;

    char to_process[MESSAGE_LENGTH] = {0};
    strcpy(to_process, in_msg);

    char* token = strtok(to_process," ");

    if(!strcmp(token, "N"))
        register_new_node(to_process + 2);
    if(!strcmp(token, "D"))
        remove_node(to_process + 2);
}

//Executed the message, input is of type message_item struct
void* process_inbound(list* in_list, li_node* input, void* data) {
    if(input == NULL) return NULL;


    char the_message[MESSAGE_LENGTH] = {0};
    //if(input->size > MESSAGE_LENGTH) return NULL;
    memcpy(the_message,input->data,input->size);

    process_message(the_message);
    li_delete_node(in_list, input);

    return NULL;
}


//Executes everything in execution queue + prunes data structures
void* inbound_executor() {
    while(true) {

        pthread_mutex_lock(&our_mutex);

        li_foreach(inbound_msg_queue, process_inbound, NULL);
        if(close_threads) {
            pthread_mutex_unlock(&our_mutex);
            return NULL;
        }
        pthread_mutex_unlock(&our_mutex);

        usleep(100);
    }
}

void mine(){
    int result;
    printf("Mining started for node: %s", our_ip);

    while(true){
        unsigned int time_start = time(NULL);
        result = proof_of_work(l_chain->head);
        unsigned int time_end = time(NULL);
    pthread_mutex_lock(&our_mutex);
    if(result==1){
        printf("Mined in %f\n", (time_end-time_start)/60);
    }
         
    pthread_mutex_unlock(&our_mutex);
    }
}

int main(int argc, const char* argv[]) {
    int ret = 0;
    
    //Ctrl-C Handler
    signal(SIGINT, shutdown);

    //load defaults
    strcpy(chain_filename, "chain_0.israft");

    //Set PoW algorithm difficulty (0xFF)
    memset(target, 0, sizeof(target));
    target[2] = 0x05;
    
    //Create our blockchain and Process chain file
    int chain_good = read_chain_from_file(l_chain, chain_filename);
    if(chain_good!=0) {
        l_chain = new_chain(); //TODO Write blockchain to file
    }
   
   //Initialization of nodes on the server
    chain_nodes = dict_create();
    //Create sockets dictionary
    out_sockets = dict_create();
    //Create list of outbound msgs & add our ip to be sent to all nodes
    outbound_msg_queue = list_create();
    //Create execution queue
    inbound_msg_queue = list_create();

    int loc = read_nodes_from_file("nodes.conf", chain_nodes);
    sprintf(our_ip, "ipc:///tmp/pipeline_%d.ipc",loc);

    //Send out our existence + START GENESIS IF NO BLOCK REPLY
    dict_foreach(chain_nodes,announce_existance, NULL); //TODO Handle receive
    last_ping = time(NULL);

    //pthread_mutex_t our_mutex = PTHREAD_MUTEX_INITIALIZER
    pthread_mutex_init(&our_mutex, NULL);
    if((ret = pthread_create(&inbound_network_thread, NULL, &in_server,NULL)) != 0) {
        printf("Error inbound_network_thread pthread_create:in");
        return ret;
    }
    if((ret = pthread_create(&outbound_network_thread, NULL, &out_server,NULL)) != 0) {//TODO handle response 
        printf("Error outbound_network_thread pthread_create:in");
        return ret;
    }
    if((ret = pthread_create(&inbound_executor_thread, NULL, &inbound_executor,NULL)) != 0) {//TODO handle response 
        printf("Error inbound_executor_thread pthread_create:in");
        return ret;
    }
    close_threads = 0;


    while(true){
        usleep(3);
    };

    
    return 0;
}
