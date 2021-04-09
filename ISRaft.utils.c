#include "ISRaft.h"

extern char this_ip[ADDRESS_SIZE];
extern int total_votes;

int add_node_to_dict(char* server_address, dict* chain_nodes){
    int ret = 0;
    raft_node* rn;
    socket_item* sock_item;

    rn = calloc(1,sizeof(raft_node));
    strcpy(rn->ip_address,server_address);
    //ADD SOCKET HANDLE
    if((ret = create_raft_socket(server_address,&sock_item))!=0){
        free(rn);
        return ret;
    }
    rn->socket_it = sock_item;
    dict_insert(chain_nodes,server_address,rn,sizeof(raft_node));
    free(rn);
    return ret;
}

//Read the nodes on server, and writes a new self address to file
int read_nodes_from_file(const char* filename, dict* chain_nodes){
    FILE* chain_file;
    socket_item* sock_item;
    char buff[DEFAULT_STR_SIZE] = {0};
    int curr_index = -1;
    char char_index[20] = {0};
    int ret = 0;

    ///File has to be with:///tmp/pipeline_0.ipc format
    log_trace("Reading nodes from file: '%s'", filename);

    chain_file = fopen(filename, "ab+");
    if(chain_file == NULL){
        log_error("Unable to create a file: %s",filename);
        return ERR_FILE;
    }

    while(fgets(buff, sizeof(buff), chain_file)){
        if(buff[strlen(buff)-1] == '\n') buff[strlen(buff)-1] = 0; //Check this line
        curr_index = atoi(buff+strlen(buff)-5);

        add_node_to_dict(buff,chain_nodes);
    }
    //handle empty file

    fprintf(chain_file, "ipc:///tmp/pipeline_%d.ipc\n",curr_index+1);
    sprintf(this_ip, "ipc:///tmp/pipeline_%d.ipc",curr_index+1);
    log_trace("This servers IP: %s", this_ip);
    fclose (chain_file);
    return ret;
}

//Create a socket to be used for the given address
int create_raft_socket(const char* input, socket_item** sock_item) {
    int timeout = 100;
    int ret = 0;
    socket_item* new_out_socket;

    if(strlen(input) > 299) return ERR_LENGTH;

    log_trace("Creating socket for: %s", input);
    new_out_socket = malloc(sizeof(socket_item));


    new_out_socket->last_used = time(NULL); //Set current time as the time last used. 
    new_out_socket->socket = nn_socket(AF_SP, NN_PUSH);

    if(new_out_socket->socket < 0) {
        free(new_out_socket);
        return ERR_SOCKET;
    }
    if(nn_setsockopt(new_out_socket->socket, NN_SOL_SOCKET, NN_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        free(new_out_socket);
        return ERR_SOCKET;
    }

    if(nn_connect (new_out_socket->socket, input) < 0){
        log_error("Socket Connection Error");
        ret = ERR_SOCKET;
        nn_close(new_out_socket->socket);
    }

    *sock_item = new_out_socket;
    return ret;
}

//Empty message init
int setup_message(message_item* in_message) {

    if(in_message == NULL) return ERR_NULL;

    in_message->tries = 0;
    memset(in_message->toWhom, 0, sizeof(in_message->toWhom));
    return 0;
}

void* send_message(list* in_list, li_node* input, void* data){
    int socket_handler;
    int used_rare_socket = 0;
    int bytes;
    message_item* new_message;
    raft_node* node;

    if(input == NULL) goto end_send_message;

    if((new_message = (message_item*)input->data) == NULL) goto end_send_message;
    if((node = (raft_node*)dict_access((dict*)data,new_message->toWhom)) == NULL) log_error("Node is NULL");

    socket_handler = node->socket_it->socket;
    bytes = nn_send (socket_handler,  new_message->message, strlen(new_message->message), 0);

    log_info("Sending to: %s, Bytes sent: %d", new_message->toWhom, bytes);

    usleep(100);

    if(bytes > 0 || new_message->tries == 2) li_delete_node(in_list, input);
    else new_message->tries++;

end_send_message:

    return NULL;
}

//Executed the message, input is of type message_item struct
void* process_inbound(list* in_list, li_node* input, void* data) {
    if(input == NULL) return NULL;

    pthread_mutex_t* the_mutex = (pthread_mutex_t*)data;

    char the_message[MESSAGE_LENGTH] = {0};
    //if(input->size > MESSAGE_LENGTH) return NULL;
    strcpy(the_message,(char*)input->data);

    pthread_mutex_lock(the_mutex);
    process_message(the_message, (int)input->size);
    li_delete_node(in_list, input);
    pthread_mutex_unlock(the_mutex);

    return NULL;
}

int check_total_votes(bt_node* in_dict, void* data){
    raft_node * rn = (raft_node*)in_dict->data;
    if(rn->voted_for_self == 1){
        total_votes++;
    }
    return 0;
}

int reset_votes(bt_node* in_dict, void* data){
    raft_node * rn = (raft_node*)in_dict->data;
    rn->voted_for_self = 0;
    total_votes = 0;
    return 0;
}