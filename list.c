


#include <stdio.h>
#include <stdlib.h>
#include <string.h>



struct mq_msg_data {
    struct mq_msg_data *next;
    char mq_msg_buf[256];
};

struct mq_msg_data *head = NULL;

static int mq_msg_push(char *msg)
{
    struct mq_msg_data *temp;
    struct mq_msg_data *node;
    //
    node = (struct mq_msg_data *)malloc(sizeof(struct mq_msg_data));
    node->next = NULL;
    memcpy(node->mq_msg_buf, msg, 256);
    // add to the list
    if (head == NULL) {
        head = node;
        return 0;
    }
    temp = head;
    while (temp->next)
        temp = temp->next;
    temp->next = node;
    //
    return 0;
}

static int mq_msg_pop(char *msg)
{
    struct mq_msg_data *temp;
    //
    if (head == NULL)
        return -1;
    memcpy(msg, head->mq_msg_buf, 256);
    temp = head;
    head = head->next;
    free(temp);
    //
    return 0;
}

static void mq_msg_show(void)
{
    struct mq_msg_data *temp = head;
    printf("-----------------------------------\n");
    while (temp) {
        printf("[%s]\n", temp->mq_msg_buf);
        temp = temp->next;
    }
}




int main(void)
{
    char buf[512] = "INITIAL";
    mq_msg_show();
    ///
    mq_msg_push("1234567890");
    mq_msg_show();
    //
    mq_msg_pop(buf);
    printf("::: %s\n", buf);
    mq_msg_show();

    /////
    mq_msg_push("1234567890");
    mq_msg_push("1234567890");
    mq_msg_show();

    /////




    return 0;
}



