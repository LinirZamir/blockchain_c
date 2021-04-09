#include "ISRaft.h"

extern char this_ip[ADDRESS_SIZE];
extern dict* chain_nodes;
extern int total_nodes;

//Regster New Node and send out your chain length
int register_new_node(char* input) {
    int ret = 0;

    if(input == NULL) return ERR_NULL;

    if(!strcmp(input, this_ip)) {
        log_warn("Someone is acting on the same IP");
        return ERR_GENERAL;
    }

    if(strlen(input) > ADDRESS_SIZE) {
        log_error("IP address too long to register!");
        return ERR_LENGTH;
    }

    log_info("Registering New Node: %s", input);

    if(dict_access(chain_nodes, input) != NULL){ //Check if node already exists
        log_warn("Node %s is already registered", input);
    }else{
        //Add to dict
        add_node_to_dict(input,chain_nodes);
        total_nodes ++;
    }   
}


//Message type: T: transaction, P: post, B: block, N: new node, L: datalog length, C: chain
void process_message(const char* in_msg, int msg_len) {
    if(in_msg == NULL) return;

    if(msg_len<60);
    char * to_process = malloc(MESSAGE_LENGTH);
    memset(to_process,0,MESSAGE_LENGTH);
    memcpy(to_process, in_msg,msg_len);

    char* token = strtok(to_process," ");
    
    if(!strcmp(token, "N"))
        register_new_node(to_process + 2);
}