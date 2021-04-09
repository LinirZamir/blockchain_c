#include "ISRaft.h"

extern char this_ip[ADDRESS_SIZE];
extern dict* chain_nodes;
extern int total_nodes;
extern raft_node* elected_node;
extern int heartbeat_timer;
extern int self_status;
extern int total_votes;

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

//Remove node from dict
int remove_node(char* input){
    if(input == NULL) return ERR_NULL;
    log_trace("Removing Node: %s",input);

    total_nodes--;
    //Remove from chain dict
    dict_del_elem(chain_nodes,input,0);
    return 0;
}

//Processing request_vote
int request_vote_process(char* input){
    if(input == NULL) return ERR_NULL;
    if(self_status != 2){
        log_trace("Node: %s requested a vote",input);
        elected_node = (raft_node*)dict_access(chain_nodes, input);
        return 0;
    }else{
        log_warn("A different node requested to be leader");
        return 1;
    }
}

//Processing request_vote
int heartbeat_process(char* input){
    raft_node* r_node;
    if(input == NULL) return ERR_NULL;
    r_node = (raft_node*)dict_access(chain_nodes, input);
    if(r_node->leader == 1)
    {    
        heartbeat_timer = ELECTION_TIMEOUT + rand() % 3000;
        self_status = 0;
    }
    else
        log_warn("Non-leader sent HB");
    return 0;
}

//Processing vote yes
int vote_yes_process(char* input){
    raft_node * new_node;
    if(input == NULL) return ERR_NULL;
    new_node = (raft_node*)dict_access(chain_nodes, input);
    new_node->voted_for_self = 1;
    dict_foreach(chain_nodes,check_total_votes,NULL);
    if((float)total_votes/(float)total_nodes >0.5){
        self_status = 2;
        //Send heartbeat message also??
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
    if(!strcmp(token, "D"))
        remove_node(to_process + 2);
    if(!strcmp(token, "R"))
        request_vote_process(to_process + 2);
    if(!strcmp(token, "H"))
        heartbeat_process(to_process + 2);
    if(!strcmp(token, "Y"))
        vote_yes_process(to_process + 2);
}