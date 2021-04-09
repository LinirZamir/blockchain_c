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

