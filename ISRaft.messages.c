#include "ISRaft.h"

extern char this_ip[ADDRESS_SIZE];
extern list* outbound_msg_queue;

//Message sent to all nodes when a new node joins
int announce_existance(bt_node* in_dict, void* data){
    if(in_dict == NULL || in_dict->size > 300) return ERR_NULL;

    message_item announcement;
    setup_message(&announcement);
    strcpy(announcement.toWhom,in_dict->key);
    strcpy(announcement.message, "N ");
    strcat(announcement.message, this_ip);

    li_append(outbound_msg_queue,&announcement,sizeof(announcement));
}

int announce_exit(bt_node* in_dict, void* data){
    if(in_dict == NULL || in_dict->size > 300) return ERR_NULL;

    message_item announcement;
    setup_message(&announcement);
    strcpy(announcement.toWhom,in_dict->key);
    strcpy(announcement.message, "D ");
    strcat(announcement.message, this_ip);

    li_append(outbound_msg_queue,&announcement,sizeof(announcement));
}

int request_vote(bt_node* in_dict, void* data){
    if(in_dict == NULL || in_dict->size > 300) return ERR_NULL;

    message_item announcement;
    setup_message(&announcement);
    strcpy(announcement.toWhom,in_dict->key);
    strcpy(announcement.message, "R ");
    strcat(announcement.message, this_ip);

    li_append(outbound_msg_queue,&announcement,sizeof(announcement));
}


int vote_yes(raft_node* in_node, void* data){
    if(in_node == NULL) return ERR_NULL;

    message_item announcement;
    setup_message(&announcement);
    strcpy(announcement.toWhom,in_node->ip_address);
    strcpy(announcement.message, "Y ");
    strcat(announcement.message, this_ip);

    li_append(outbound_msg_queue,&announcement,sizeof(announcement));
}


int heartbeat(bt_node* in_dict, void* data){
    if(in_dict == NULL || in_dict->size > 300) return ERR_NULL;

    message_item announcement;
    setup_message(&announcement);
    strcpy(announcement.toWhom,in_dict->key);
    strcpy(announcement.message, "H ");
    strcat(announcement.message, this_ip);

    li_append(outbound_msg_queue,&announcement,sizeof(announcement));
}

