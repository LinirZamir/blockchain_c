#include "blockchain.h"

char our_ip[300];
int last_check;

dict* chain_nodes;
list* outbound_msg_queue; 
list* inbound_msg_queue;

//Socket
dict* out_sockets;

//Threads
pthread_t inbound_network_thread;
pthread_t outbound_network_thread;
pthread_mutex_t our_mutex;

//Read the nodes on server, and writes a new self address to file
int read_othernodes_from_file(const char* filename, dict* dict_nodes){
    ///File has to be in ipc:///tmp/pipeline_0.ipc format
    printf("Reading nodes from file: '%s'\n", filename);
    FILE* chain_file = fopen(filename, "ab+");
    if(chain_file == NULL) return ERR_FILE;
    
    char buff[BLOCK_STR_SIZE] = {0};
    int curr_index = 0;
    while(fgets(buff, sizeof(buff), chain_file)){
        if(buff[strlen(buff)-1] == '\n') buff[strlen(buff)-1] = 0; //Check this line
        printf("Reading from file: %s\n", buff);
        dict_insert(dict_nodes,buff,"datainside",strlen("datainside"));
        create_socket(buff);
    }
    fclose (chain_file);
    return ERR_FILE;
}

void display_help(){
    printf("Help/Commands: 'h'\n"); //Show Help
    printf("Test connection: 't'\n"); //Show Help
    printf("Transactions: 'r'\n");  //Show transactions on blockchain
    printf("Post transaction: 'p sender amount receiver'\n"); //Send currency between nodes
    printf("Quit program: 'q'\n"); //Quit
}

void quit_program(){
    printf("Thank you for using the program!/nShutting down...\n");
    exit(0);
}

int announce_test(bt_node* in_dict, void* data){
    if(in_dict == NULL || in_dict->size > 300) return ERR_NULL;

    message_item announcement;
    setup_message(&announcement);
    strcpy(announcement.toWhom,in_dict->key);
    strcpy(announcement.message, "T ");
    strcat(announcement.message, our_ip);

    li_append(outbound_msg_queue,&announcement,sizeof(announcement));
}

void test_connection(){
    printf("Test connection - send ping to all nodes\n");
    dict_foreach(chain_nodes,announce_test, NULL);
}

void* process_outbound(list* in_list, li_node* input, void* data){
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

//Outbound thread function - tried to send everything in outbound message queue
void* out_server() {
    last_check = time(NULL);
    while(true) {
        pthread_mutex_lock(&our_mutex);
        li_foreach(outbound_msg_queue, process_outbound, NULL);

        //Check on un verified messages
        pthread_mutex_unlock(&our_mutex);
        usleep(10);
    }
}

int main(){
    int ret = 0;
    char buffer[120] = "";

    printf("Blockchain implementation in C\n'h' for help/commends\n");

    //Initialization of nodes on the server
    chain_nodes = dict_create();
    //Create list of outbound msgs & add our ip to be sent to all nodes
    outbound_msg_queue = list_create();
    //Create execution queue
    inbound_msg_queue = list_create();
    //Create socket
    out_sockets = dict_create();

    strcpy(our_ip, "ipc:///tmp/pipeline_001.ipc");

    ret = read_othernodes_from_file("nodes.conf", chain_nodes);


    //Threads
    pthread_mutex_t our_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_create(&outbound_network_thread, NULL, out_server, NULL);

    while(true){
        printf("ISRaft>");
        fgets(buffer, 120, stdin);
        buffer[strlen(buffer) - 1] = 0;
        if(!strcmp("h", buffer) || !strcmp("H", buffer) || !strcmp("help", buffer))
            display_help();
        if(!strcmp("q", buffer) || !strcmp("Q", buffer) || !strcmp("quit", buffer))
            quit_program();
        if(!strcmp("t", buffer) || !strcmp("T", buffer) || !strcmp("test", buffer))
            test_connection();
    }

    return 0;
}