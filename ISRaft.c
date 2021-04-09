#include "ISRaft.h"

//Data
dict* chain_nodes;
list* outbound_msg_queue; //holds outbound message structs
list* inbound_msg_queue; //holds recieved char* messages to execute
int total_nodes = 1;

//Self Properties
char this_ip[ADDRESS_SIZE] = {0};
int socket_in;

//Threads
pthread_mutex_t our_mutex;
pthread_t inbound_network_thread;
pthread_t outbound_network_thread;
pthread_t inbound_executor_thread;
int close_threads;

void shutdown(int dummy) {

    log_info("Commencing shutdown!");

    close_threads = 1;

    pthread_join(outbound_network_thread, NULL);
    pthread_join(inbound_network_thread, NULL);
    pthread_join(inbound_executor_thread, NULL);

    log_info("Threads Joined!");

    pthread_mutex_lock(&our_mutex);

    //Discard lists
    li_discard(outbound_msg_queue);
    li_discard(inbound_msg_queue);

    //Discard keys
    nn_close(socket_in);
    pthread_mutex_unlock(&our_mutex);
    pthread_mutex_destroy(&our_mutex);

    /*BIO_free_all(bp_public);
	BIO_free_all(bp_private);
	RSA_free(rsa);
    BN_free(bn);*/

    log_info("Shutdown complete!");
    exit(0);
}

int main(int argc, const char* argv[]) {
    int ret;

    //TODO - Create an RSA keypair

    //Ctrl-C Handler
    signal(SIGINT, shutdown);
    //TODO - handle new chain (with import and creation)

    /*Initializing dictionaries and lists*/
    //Initialization of nodes on the server
    chain_nodes = dict_create();
    //Create list of outbound msgs & add our ip to be sent to all nodes
    outbound_msg_queue = list_create();
    //Create execution queue
    inbound_msg_queue = list_create();
    if((ret = read_nodes_from_file("nodes.conf", chain_nodes))!= 0){
        log_error("Error number: %d", ret);
        return ret;
    }

    //Announce existence to all other nodes
    if(dict_foreach(chain_nodes,announce_existance, NULL) == 0) {
        log_info("This is the first node in the system");
    }

    //THREADS START
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
    
    while(true);

    return 0;
}
