// ref: https://gist.github.com/reddragon/e8a706d527bb77822ab3 http://blog.gaurav.im/2014/12/15/a-gentle-intro-to-libevent/ https://github.com/terz99/semester-3-hws/blob/master/OS/hw5/connections.c   
#include <stdio.h>  
#include <string.h>   //strlen  
#include <stdlib.h>  
#include <errno.h>  
#include <unistd.h>   //close  
#include <arpa/inet.h>    //close  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros  
/* Libevent. */
#include <event2/event.h>
static char chars_POSIX[] = " \t\r\n\v\f.,;!~`_-"; 
static const char *progname = "guess-server";
#define PORT 8932
#define MAX_CLIENTS 30

#define QUIT 0
#define COR 1
#define INCOR 2
#define NEWCHAL 3
#define WRONG 4
static struct event_base *eb;


typedef struct state {
int g_count;
int corr_count;
char answer[64];
char question[256];
} state_t;

typedef struct node{
    int fd;
    state_t *cur_state;
    struct event *cur_event;
    struct node *next;
} cnode;

static cnode* active_connections = NULL;

cnode* add_node(cnode **head, int cfd){
   cnode *curr_node = NULL;
   state_t *curr_state = NULL;
   printf("%d\n",*head);
   if(*head == NULL){
       
      *head = (cnode*) malloc(sizeof(cnode));
      if(*head == NULL){
         fprintf(stderr, "error creating new connection: %s", strerror(errno));
         exit(EXIT_FAILURE);
      }
      curr_node = *head;
   } else {
      while(curr_node->next != NULL){
         curr_node = curr_node->next;
      }
      curr_node->next = (cnode*) malloc(sizeof(cnode));
      curr_node = curr_node->next;
      curr_node->next = NULL;
   }
    
   curr_node->fd = cfd;
   curr_node->cur_event = NULL;
   curr_node->cur_state = (state_t*) malloc(sizeof(state_t));

   curr_state = curr_node->cur_state;
   curr_state->g_count = 0;
   curr_state->corr_count = 0;
   curr_state->answer[0] = '\0';
    curr_state->question[0] = '\0';

   return curr_node;
}

int delete_node(cnode **head, int cfd){
   cnode *curr_node = NULL;
   if(*head == NULL){
      fprintf(stderr, "the connection list is empty\n");
      return EXIT_FAILURE;
   }

   curr_node = *head;
   if(curr_node->fd == cfd){
      *head = ((*head)->next) ? (*head)->next : NULL;
      event_free(curr_node->cur_event);
      free(curr_node);
      return EXIT_SUCCESS;
   }

   while(curr_node->next != NULL){
      if(curr_node->next->fd == cfd){
         cnode* tmp = curr_node->next;
         curr_node->next = (curr_node->next->next) ? curr_node->next->next : NULL;
         event_free(tmp->cur_event);
         free(tmp);
         break;
      }
      curr_node = curr_node->next;
   }

   return EXIT_SUCCESS;
}

ssize_t
trans_write(int fd, const void *buf, size_t count)
{
    size_t total = 0;
    while (count > 0) {
	    int nwritten = write(fd, buf, count);///FAIL HERE
	    if (nwritten < 0 && errno == EINTR)
	        continue;
	    if (nwritten < 0)
	        return nwritten;
	    if (nwritten == 0)
	        return nwritten;
	    buf = (unsigned char *) buf + nwritten;
	    count -= nwritten;
	    total += nwritten;
    }

    return total;
}

static void write_client(cnode* cur_con, int type){
    char mess[256];
    int length;
    switch(type){
        case QUIT:
            sprintf(mess, "M: You mastered %d/%d challenges. Good bye!\n",cur_con->cur_state->corr_count,cur_con->cur_state->g_count);
            break;
        case COR:
            sprintf(mess, "Congratulation - challenge passed!\n");
            break;
        case INCOR:
            sprintf(mess,"Wrong guess - expected: %s\n",cur_con->cur_state->answer);
            break;
        case WRONG:
            sprintf(mess, "Unknown command or format\n");
            break;
        case NEWCHAL:
            sprintf(mess,"%s\n",cur_con->cur_state->question);
            break;
        default:
            break;   
    }
        printf("read questionFINAL %d %d\n",cur_con->fd,eb);
    length = trans_write(cur_con->fd,mess,strlen(mess));
    printf("read questionFINAL\n");
    if (length<0)
    {
        perror("Error while writing");
        exit(EXIT_FAILURE);
    }
}
static void choose_word(state_t* curr_state){
   char *pos;
   char *token = NULL;
   int n, word_pos;
   char tmp[64];
   pos = curr_state->question;
   while((pos = strpbrk(pos, chars_POSIX)) != NULL){
      pos++;
      n++;
   }
    
   while(token == NULL){
    srand(time(NULL));
      word_pos = rand()%n;
      n = 0;
      strcpy(tmp, curr_state->question);
      token = strtok(tmp, chars_POSIX);
      while(token){
         if(n == word_pos){
            pos = strstr(curr_state->question, token);
            if(!pos){
               break;
            }
            memset(pos, '_', strlen(token));
            strcpy(curr_state->answer, token);
            break;
         }
         n++;
         token = strtok(NULL, chars_POSIX);
      }
   }
}

static void read_fortune(evutil_socket_t evfd, short evwhat, void *evarg){
    
   int cfd = evfd;
   int length, rc;
   cnode *curr_conn = (cnode*) evarg;
   char buf[64];
    
   length = read(cfd, buf, 32);
   
   if(length < 0){
      perror("ERROR WHILE READING"); 
      exit(EXIT_FAILURE);
   } else {
      buf[length] = '\0';
      printf("read question\n");
      strcpy(curr_conn->cur_state->question, buf);
   }
    
   //choose_word(curr_conn->cur_state);
   
   write_client(curr_conn, NEWCHAL);
}

static void get_question(cnode *cur_cnode){
    printf("get question\n");
    int pipes[2];
    pid_t pid;
    struct event *ev;
    (void) pipe(pipes);
    ev = event_new(eb, pipes[0], EV_READ, read_fortune, cur_cnode);
    event_add(ev, NULL);
    pid = fork();
    if(pid == -1){
        perror("failure of fork");
        cur_cnode->cur_state->question[0] = '\0';
    } else if(pid == 0){
      close(pipes[0]);
      dup2(pipes[1], STDOUT_FILENO);
      close(pipes[1]);

      execlp("fortune", "fortune", "-n", "32", "-s", NULL);
      exit(EXIT_FAILURE);
   } else {
      close(pipes[1]);
   }
}

static void read_client(evutil_socket_t fd, short what, void* evarg){
    printf("Reading client\n");
    int cfd = fd, length=0;
    cnode *curr_cnode = (cnode*)evarg;
    char buf[256];
    if (length = read(cfd,buf,sizeof(buf))<0){
        perror("error while reading");
        exit(EXIT_FAILURE);
    }
    char *res = strstr(buf,"Q:");
    //FAIL HERE
    if (strcmp(res,buf)==0)
    {
        write_client(curr_cnode,QUIT);
                printf("fail1\n");

        event_free(curr_cnode->cur_event);
        close(cfd);
        delete_node(&active_connections, cfd);
    }
    else{//ANSWER
        if (strcmp(strstr(buf,"R:"),buf)==0)
        {
            curr_cnode->cur_state->g_count++;
            const char *temp = buf+3;
            if (strcmp(temp,curr_cnode->cur_state->answer)){
                curr_cnode->cur_state->corr_count++;//Correct
                write_client(curr_cnode, COR);
            }
            else {
                write_client(curr_cnode, INCOR);
            }
            get_question(curr_cnode);
        }
        write_client(curr_cnode,WRONG);
    }
}





static void set_connection(evutil_socket_t fd, short what, void* evarg){
    int new_sockfd = accept(fd, NULL, 0);
    char welcome[256];
    if(new_sockfd < 0){
        fprintf(stderr, "%s: could not connect: %s\n", progname, strerror(errno));
        exit(EXIT_FAILURE);
    }
    printf("Setting up the connection.\n");
    strcpy(welcome, "M: Guess the missing ____!\n");
    strcat(welcome, "M: Send your guess in the for 'R: word\\r\\n'.\n");
    printf("%s\n",welcome);
    int n = trans_write(new_sockfd, welcome, strlen(welcome));
    if(n < 0){
        fprintf(stderr, "%s: %s writing failed\n", progname, welcome);
        
        close(fd);
        exit(EXIT_FAILURE);
    }
    cnode *connection;
    connection = add_node(&active_connections, fd);


    connection->cur_event = event_new(eb, new_sockfd, EV_READ, read_client, connection);
    event_add(connection->cur_event, NULL);
    get_question(connection);
}

int main(int argc, char const* argv[])
{
    int server_fd, client_fd[MAX_CLIENTS], max_fd, activity, new_socket, addrlen, cfd;
    struct sockaddr_in address;
    char *message = "ECHO Daemon v1.0 \r\n";
    
    fd_set clients;

    char buffer[1025];  //data buffer of 1K  

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
    { 
        perror("socket failed"); 
        exit(EXIT_FAILURE); 
    } 

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd,(struct sockaddr *)&address,sizeof(address))<0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd,3)<0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    printf("SUccess for listening\n");
    addrlen = sizeof(address);
    eb = event_base_new();
    if(!eb){
        perror("creating event base failed");
        exit(EXIT_FAILURE);
    }
    struct event* t = event_new(eb, server_fd, EV_READ | EV_PERSIST, set_connection, NULL);
    event_add(t, NULL);
    if (event_base_loop(eb,0) == -1)
    {
        perror("event loop failed");
        event_base_free(eb);
        exit(EXIT_FAILURE);
    }
    event_base_dispatch(eb);
    return 0;
}



