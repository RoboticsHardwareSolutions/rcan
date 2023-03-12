#if defined(RCAN_WINDOWS) || defined(RCAN_MACOS) || defined(RCAN_UNIX)

#include "zmq_can.h"
#include "stdio.h"
#include "string.h"
#include "zmq.h"


int start ()
{
  void* context = zmq_ctx_new();
  void* publisher = zmq_socket(context, ZMQ_PUB);
  printf("Starting server...\n");
  int conn = zmq_bind(publisher, "tcp://*:4040");
  const char* companies[2] = {"Company1", "Company2"};
  int count = 0;
  for(;;)
  {
    int price = count % 2;
    int which_company = count % 2;
    int index = strlen(companies[0]);
    char update[12];
    snprintf(update, sizeof update, "%s",
             companies[which_company]);
    zmq_msg_t message;
    zmq_msg_init_size(&message, index);
    memcpy(zmq_msg_data(&message), update, index);
    zmq_msg_send(&message, publisher, 0);
    zmq_msg_close(&message);
    count++;
  }
  zmq_close(publisher);
  zmq_ctx_destroy(context);
  return 0;
}


#endif // defined(RCAN_WINDOWS) || defined(RCAN_MACOS) || defined(RCAN_UNIX)