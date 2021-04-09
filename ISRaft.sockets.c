#include "ISRaft.h"

extern int socket_in;
extern char this_ip[ADDRESS_SIZE];
extern pthread_mutex_t our_mutex;
extern list* outbound_msg_queue;
extern list* inbound_msg_queue;
extern dict* chain_nodes;
extern int close_threads;

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

    while(true) {
        pthread_mutex_lock(&our_mutex);
        pthread_mutex_t message_mutex = PTHREAD_MUTEX_INITIALIZER;

        li_foreach(outbound_msg_queue, send_message, (void*)chain_nodes);
        

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