#include "ISRaft.h"

extern int socket_in;
extern char this_ip[ADDRESS_SIZE];
extern pthread_mutex_t our_mutex;
extern list* outbound_msg_queue;
extern list* inbound_msg_queue;
extern dict* chain_nodes;
extern int close_threads;
extern int heartbeat_timer;
extern int self_status;
extern raft_node* elected_node;

//Inbound thread function
void* in_server(){
    int timeout = TIMEOUT;
    char buf[MESSAGE_LENGTH];

    socket_in = nn_socket (AF_SP, NN_PULL);
    assert(socket_in >= 0);
    assert(nn_bind(socket_in, this_ip) >= 0);
    assert(nn_setsockopt(socket_in, NN_SOL_SOCKET, NN_RCVTIMEO, &timeout, sizeof(timeout))>=0);

    log_info("Inbound Socket Ready!");

    while(true) {
        int bytes = nn_recv(socket_in, buf, sizeof(buf), 0);
        
        pthread_mutex_lock(&our_mutex);
        if(bytes > 0) {
            buf[bytes] = '\0';
            log_info("Recieved %d bytes: \"%s\"", bytes, buf);
            li_append(inbound_msg_queue,buf,bytes);
        }
        if(close_threads) {
                pthread_mutex_unlock(&our_mutex);            
                return NULL;
        }
        pthread_mutex_unlock(&our_mutex);            
    }
    return NULL;
}

//Outbound thread function
void* out_server() {
    time_t t;

    while(true) {

        pthread_mutex_lock(&our_mutex);
        pthread_mutex_t message_mutex = PTHREAD_MUTEX_INITIALIZER;
        srand((unsigned) time(&t)); // Handle randomness for message queue (requestvote)
        li_foreach(outbound_msg_queue, send_message, (void*)chain_nodes);
        
        //Handle message_timer
        heartbeat_timer--;
        //Election for current node follower
        if(heartbeat_timer == 0 && self_status == 0){
            //Send requestvote
            log_warn("Becoming a candidate");
            if(dict_foreach(chain_nodes,request_vote, NULL) == 0) {
                log_warn("There are no other nodes to accept leader");
                self_status = 0; //Stay follower until more nodes join
            }else{
                if(elected_node!=NULL) elected_node->leader=0;
                self_status = 1;
            }
            heartbeat_timer = ELECTION_TIMEOUT + rand() % 3000;
        }
        //Election for current node candidate
        if(heartbeat_timer == 0 && self_status == 1){
            log_warn("Election is over");
            if(elected_node == NULL) {
                log_error ("No node was elected");
            }else{
                log_info("Voted YES for node: %s", elected_node->ip_address);
                
                vote_yes(elected_node, NULL);
                elected_node->leader = 1; //Set flag as leader
            }
            self_status = 0;
            heartbeat_timer = ELECTION_TIMEOUT + rand() % 3000;
        }

        //Sending Heartbeat messages as leader
        if(self_status == 2){
             if(dict_foreach(chain_nodes,heartbeat, NULL) == 0) {
                log_warn("There are no other nodes to accept heartbeat");
            }
            heartbeat_timer = ELECTION_TIMEOUT + rand() % 3000;
        }

        if(close_threads) {
            pthread_mutex_unlock(&our_mutex);
            return NULL;
        }
        pthread_mutex_unlock(&our_mutex);

        usleep(500);
    }
}

//Executes everything in execution queue
void* inbound_executor() {
    while(true) {


        if(inbound_msg_queue->length>0){
            li_foreach(inbound_msg_queue, process_inbound, &our_mutex);
        }

        pthread_mutex_lock(&our_mutex);


        if(close_threads) {
            pthread_mutex_unlock(&our_mutex);
            return NULL;
        }
        pthread_mutex_unlock(&our_mutex);

        usleep(100);
    }
}