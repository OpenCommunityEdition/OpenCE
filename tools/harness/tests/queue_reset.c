#include "harness.h"
#undef match_assert
#define match_assert(file,line,condition) CHECK(condition,"%s:%d",file,line)

#define MAX(a,b) ((a)>(b)?(a):(b))
#define csmemset memset
#define MAXIMUM_CLIENT_UPDATES 8
struct player_action { byte bytes[32]; };
struct update { long update_number; struct { short action_count; struct player_action actions[4]; } update; };
struct update_server_queue_datum { short identifier, pad; long next_update_number; struct player_action current_action; };
struct data_iterator { struct data_array *data; long slot, datum_index; };
static struct data_array *player_data;
static struct { boolean initialized; long next_update_number_to_build; struct data_array *queues; struct update updates[16]; } update_server_globals;
static struct { boolean initialized; long next_update_number_to_dequeue, latest_update_number_received;
 struct player_action saved_action_collection[4]; struct data_array *queues; struct update updates[8]; } update_client_globals;
static long now;
static int distributed_resets;

static void data_make_valid(struct data_array *array) { (void)array; }
static void data_delete_all(struct data_array *array) { memset(array->data,0,array->size*array->maximum_count); array->count=0; }
static long datum_new_at_index(struct data_array *array,long index)
{
 struct datum_header *header=datum_get(array,index);
 CHECK(!header->identifier,"queue occupied"); memset(header,0,array->size); header->identifier=(short)(index>>16);
 array->count=MAX(array->count,(long)DATUM_INDEX_TO_ABSOLUTE_INDEX(index)+1); return index;
}
static void data_iterator_new(struct data_iterator *iterator,struct data_array *array)
{ iterator->data=array; iterator->slot=0; iterator->datum_index=NONE; }
static void *data_iterator_next(struct data_iterator *iterator)
{
 while(iterator->slot<iterator->data->maximum_count)
 {
  long slot=iterator->slot++;
  struct datum_header *header=(struct datum_header *)(iterator->data->data+slot*iterator->data->size);
  if(header->identifier) { iterator->datum_index=((long)(unsigned short)header->identifier<<16)|slot; return header; }
 }
 return NULL;
}
static void *checked_datum_get(struct data_array *array,long index)
{
 struct datum_header *header=datum_get(array,index);
 CHECK(header->identifier,"lookup of empty queue slot %ld",index); return header;
}
#define datum_get checked_datum_get
static void update_queues_distributed_reset(void) { distributed_resets++; }
static long game_time_get(void) { return now; }
#include "under_test.inc"

static void check_reset(int first,int last,boolean server,boolean client,long time)
{
 int i; struct data_array *queues;
 memset(&update_server_globals,0,sizeof(update_server_globals)); memset(&update_client_globals,0,sizeof(update_client_globals));
 player_data=game_state_data_new("players",128,sizeof(struct datum_header));
 update_server_globals.queues=game_state_data_new("server",128,sizeof(struct update_server_queue_datum));
 update_client_globals.queues=game_state_data_new("client",128,sizeof(struct datum_header));
 for(i=first;i<=last;i++)datum_new_at_index(player_data,(long)((0x8001UL+i)<<16)|i);
 update_server_globals.initialized=server; update_client_globals.initialized=client; now=time; distributed_resets=0;
 update_server_globals.next_update_number_to_build=999; update_client_globals.next_update_number_to_dequeue=999;
 update_queues_reset_and_fill_with_lies();
 CHECK(distributed_resets==1,"distributed inputs were not reset");
 CHECK(update_server_globals.next_update_number_to_build==(client?time:(server?0:999)),"wrong server clock");
 if(client)CHECK(update_client_globals.next_update_number_to_dequeue==time && update_client_globals.latest_update_number_received==time-1,"wrong client clock");
 queues=update_server_globals.queues;
 for(i=0;i<128;i++)
 {
  struct update_server_queue_datum *queue=(void *)(queues->data+i*queues->size);
  CHECK(!!queue->identifier==(server&&i>=first&&i<=last),"wrong queue membership at %d",i);
  if(queue->identifier)CHECK(queue->next_update_number==(client?time:0),"queue %d clock %ld",i,queue->next_update_number);
 }
 if(client)
 {
  long start=MAX(0,time-MAXIMUM_CLIENT_UPDATES);
  for(i=0;start<time;i++,start++)CHECK(update_client_globals.updates[i].update_number==start && update_client_globals.updates[i].update.action_count==1,"bad synthetic client update");
 }
 free(player_data->data);free(player_data);free(queues->data);free(queues);
 free(update_client_globals.queues->data);free(update_client_globals.queues);
}
int main(int argc,char **argv)
{
 int i; char const *case_name=argc==2?argv[1]:"";
 CASE("empty")check_reset(1,0,TRUE,TRUE,1200);
 else CASE("sparse") { for(i=1;i<128;i++)check_reset(i,i,TRUE,TRUE,1200); }
 else CASE("slot-zero")check_reset(0,0,TRUE,TRUE,1200);
 else CASE("eight")check_reset(0,7,TRUE,TRUE,1200);
 else CASE("full")check_reset(0,127,TRUE,TRUE,1200);
 else CASE("dedicated")check_reset(1,7,TRUE,TRUE,1200);
 else CASE("client-only")check_reset(1,7,FALSE,TRUE,1200);
 else CASE("uninitialized")check_reset(1,7,FALSE,FALSE,1200);
 else CASE("early") {check_reset(1,7,TRUE,TRUE,0);check_reset(1,7,TRUE,TRUE,3);}
 else return 2;
 return 0;
}
