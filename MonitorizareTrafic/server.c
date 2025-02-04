#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <signal.h>
#include "myMap.h"
#include <curl/curl.h>
#include <cjson/cJSON.h>
#include <time.h>
#include <sqlite3.h>

#define PORT 2909

void cleanup_and_exit(int sig);
void setup_signal_handler();

extern int errno;

struct ResponseData {
    char *data;
    size_t size;
};

int InitializeDB();
size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp);
int RetreiveOrAddUser(const char *name, const char* pass, char* pref, int changePref);

void GetWeather(const char * city, char * resTxt);
void GetSport(char * resTxt);

static void *treat(void *); /* functia executata de fiecare thread ce realizeaza comunicarea cu clientii */
void handle_entry(const char *key, void *value);

typedef struct {
	pthread_t idThread; //id-ul thread-ului
	int thCount; //nr de conexiuni servite
}Thread;

void send_notification(const char *key, void *value, const char *msg);

typedef struct {
   char ID[100];
   int client;
   int threadID;
   char street[100];
   char flags[100];
   char name[100];
   char password[100];
   float speed;
}Client;

typedef struct {
   int maxSpeed;
   int accident;
}Street;

HashMap *clientMap;
HashMap *allStreets;

Thread *threadsPool;

int sd;
int nthreads;
pthread_mutex_t mlock=PTHREAD_MUTEX_INITIALIZER;

int respond(int cl,int idThread);
void setup_streets();

int main (int argc, char *argv[])
{

  printf("%d\n", InitializeDB());

  srand(time(NULL));
  setup_signal_handler();

  clientMap = create_hashmap();
  allStreets = create_hashmap();
  setup_streets();
  
  struct sockaddr_in server; 	
  void threadCreate(int);

   if(argc<2)
   {
        fprintf(stderr,"Eroare: Primul argument este numarul de fire de executie...");
	exit(1);
   }
   nthreads=atoi(argv[1]);
   if(nthreads <=0)
   {
        fprintf(stderr,"Eroare: Numar de fire invalid...");
	exit(1);
   }
    threadsPool = calloc(sizeof(Thread),nthreads);

  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("[server]Eroare la socket().\n");
      return errno;
    }

  int on=1;
  setsockopt(sd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
  setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));

  bzero (&server, sizeof (server));

  server.sin_family = AF_INET;	
  server.sin_addr.s_addr = htonl (INADDR_ANY);
  server.sin_port = htons (PORT);
  
  if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1)
  {
      perror ("[server]Eroare la bind().\n");
      return errno;
  }


  if (listen (sd, 10) == -1)
  {
      perror ("[server]Eroare la listen().\n");
      return errno;
  }

  printf("Nr threaduri %d \n", nthreads); fflush(stdout);
  int i;
  for(i=0; i<nthreads;i++) threadCreate(i);

	
  for (;;) 
  {
     //printf ("[server]Asteptam la portul %d...\n",PORT);
     sleep(5);
     //iterate(clientMap, handle_entry);			
  }
};
				
void threadCreate(int i)
{
   void *treat(void *);
	
   pthread_create(&threadsPool[i].idThread,NULL,&treat,(void*)i);
   return;
}

void *treat(void * arg)
{		
   	
   int client;		        
   struct sockaddr_in from; 
   bzero (&from, sizeof (from));
   
   
   printf ("[thread]- %d - pornit...\n", (int) arg);fflush(stdout);
   
   for(;;)
   {
      int length = sizeof (from);
      if ((client = accept (sd, (struct sockaddr *) &from, &length)) < 0)
      {
         perror ("[thread]Eroare la accept().\n");	  			
      }
      
      while(respond(client,(int)arg) != -1);
      close (client);
      threadsPool[(int)arg].thCount++;
   }   							
}


int respond(int cl,int idThread)
{
   char msg[200], msgResp[200], tmp[200] = "", id[100] = "", cmm[100] = "", street[100] = "", flags[100] = "", city[100] = "\0"; 
   int i, t, speed;
   
   bzero(msg, sizeof(msg));
   bzero(msgResp, sizeof(msgResp));
   
   if (read (cl, msg,sizeof(msg)) <= 0)
   {
      printf("[Thread %d]\n",idThread);
      return -1;			
   }		      
   msg[strcspn(msg, "\n")] = '\0';
   
   printf("[Thread %d]Message received [%s].\n", idThread, msg);
   if(strlen(msg) == 0) return -1;
   //get id from message
   i = 0;
   while(i < strlen(msg) && msg[i++] != '-');
   strncpy(id, msg, i-1);
   printf("[Thread %d]Extracted id [%s].\n", idThread, id);
   
   t = i;
   while(i < strlen(msg) && msg[i++] != '-');
   strncpy(cmm, msg + t, i - t - 1);   
   printf("[Thread %d]Extracted command C:[%s].\n", idThread, cmm);
   
   if(strcmp(cmm, "setup") == 0)
   {      
      Client *CL = malloc(sizeof(Client));
      strcpy(CL->ID, id);
      CL->client = cl;
      CL->threadID = idThread;
      char usrnm[100] = "\0", passwd[100] = "\0", pref[20] = "\0";
      
      t = i;
      while(i < strlen(msg) && msg[i++] != '-');
      strncpy(usrnm, msg + t, i - t - 1);
      
      t = i;
      while(i < strlen(msg) && msg[i++] != '-');
      strncpy(passwd, msg + t, i - t - 1);
      
      t = i;
      while(i < strlen(msg) && msg[i++] != '-');
      strncpy(city, msg + t, i - t - 1);
      
      int d = RetreiveOrAddUser(usrnm, passwd, pref, 0);
      strcpy(CL->name, usrnm);
      strcpy(CL->password, passwd);
      
      if(d == 1)
      {
         sprintf(msgResp + strlen(msgResp), "[setFlags]%s|", pref);
         strcmp(CL->flags, pref);
      }
      
      pthread_mutex_lock(&mlock);
      insert(clientMap, (const char*)id, CL);
      pthread_mutex_unlock(&mlock);
      
      sprintf(msgResp + strlen(msgResp), "Succesfully connected %s.", id);
   }
   else if(strcmp(cmm, "accident") == 0)
   {      
      sprintf(msgResp, "Accident set on your street.|");
      int res = 0;
      Client *thisCl = (Client*)get(clientMap, (const char*) id, &res);
                         
      printf("[Thread %d]Looked for client with id [%s] with result [%d].\n", idThread, id, res);
      
      if(res == 1)
      {
         sprintf(tmp,"%s" , thisCl->street);
         printf("[Thread %d]Setup accident notif [%s].\n",idThread, (const char *)tmp);
         iterateM(clientMap, (const char *)tmp, send_notification);

         sprintf(msgResp+ strlen(msgResp), "Got accident info for client with id [%s].", id);         
      }
      else
      {
         sprintf(msgResp+ strlen(msgResp), "Failed to get accident info for client with id %s.", id);
      } 
      
   }
   else if(strcmp(cmm, "info") == 0) 
   {    
      t = i;
      while(i < strlen(msg) && msg[i++] != '-');
      strncpy(street, msg + t, i - t - 1);
      
      t = i;
      while(i < strlen(msg) && msg[i++] != '-');
      strncpy(tmp, msg + t, i - t - 1);
      speed = atoi(tmp);
      
      t = i;
      while(i < strlen(msg) && msg[i++] != '-');
      strncpy(flags, msg + t, i - t - 1);
      
      t = i;
      while(i < strlen(msg) && msg[i++] != '-');
      strncpy(city, msg + t, i - t - 1);
      
      printf("[Thread %d]Extracted info street [%s] speed [%d] and flags [%s].\n", idThread, street, speed, flags);
      
      int res = 0;
      Client *thisCl = (Client*)get(clientMap, (const char*) id, &res);
      
      RetreiveOrAddUser(thisCl->name, thisCl->password, flags, 1);
                     
      printf("[Thread %d]Looked for client with id [%s] with result [%d].\n", idThread, id, res);     
      if(strstr(flags, "Weather") != NULL)
      {
         char wth[30] = "sunny\0";
         GetWeather(city, wth);
         printf("GOT WEATHER: %s\n", wth);
         sprintf(msgResp + strlen(msgResp), "{W}%s|", wth);
      }
      if(strstr(flags, "Sport") != NULL)
      {
         char sprt[30] = "soccer\0";
         GetSport(sprt);
         printf("GOT SPORT: %s\n", sprt);
         sprintf(msgResp + strlen(msgResp), "{S}%s|", sprt);
      }
      if(strstr(flags, "Gas") != NULL)
      {
         char peco[50] = "\0";
         GetPecoID(city, peco);
         printf("GOT PECO: %s\n", peco);
         sprintf(msgResp+ strlen(msgResp), "{G}%s|", peco);
      }
      
      if(res == 1)
      {
         strcpy(thisCl->street, street);
         thisCl->speed = speed;
         strcpy(thisCl->flags, flags);
         //sprintf(msgResp, "Got update for client with id %s", id);         
      }
      else
      {
         sprintf(msgResp + strlen(msgResp), "Could not get update for client with id %s", id); 
      }   
      
      res = 0;
      int *maxSpeed = (int*)get(allStreets, (const char*)street, &res);
      
      printf("[Thread %d]Looked for street with name [%s] with result [%d].\n", idThread, street, res);   
      
      if(res == 1)
      {
         printf("[Thread %d] Got speed for street [%s] as [%d] and client speed is [%d].\n", idThread, street, *maxSpeed, speed);
         if(*maxSpeed < speed)
            sprintf(msgResp + strlen(msgResp), "[ATTENTION] Illegal speed. Slow down from [%d] to [%d].", speed,  *maxSpeed); 
         else
            sprintf(msgResp+ strlen(msgResp), "[ATTENTION] Legal speed. Keep under [%d].", *maxSpeed); 
      }
      else
      {
         sprintf(msgResp+ strlen(msgResp), "Could not get info for client street with id %s", id); 
      }  
   }
   else 
   {
      sprintf(msgResp, "Incorect command {%s}", cmm);
   }
   
   strcat(msgResp, "|");
   
   pthread_mutex_lock(&mlock);      			      
   if (write (cl, msgResp, strlen(msgResp)) <= 0)
   {
      printf("[Thread %d] ",idThread);
      perror ("[Thread]Eroare la write() catre client.\n");
   }
   else
      printf ("[Thread %d]Message [%s] sent successfully.\n",idThread, msgResp);	
   pthread_mutex_unlock(&mlock);
   return 0;

}

void handle_entry(const char *key, void *value) 
{
    printf("Key: %s, Value: %s\n", key, (char *)value);
}

void send_notification(const char *key, void *value, const char *msg) 
{
    printf("[Thread ?]Getting client [%s] for notif [%s].\n", key, msg);
    int res = 0;
    Client *thisCl = (Client*)get(clientMap, key, &res);
    char tmp[100] = "";
    printf("[Thread ?]Got client [%s] for notif [%s] with result [%d].\n", key, msg, res);
    if(res == 1 && strcmp(msg, thisCl->street) == 0)
    {
       pthread_mutex_lock(&mlock);
       sprintf(tmp, "[A]Accident reported on %s.|", msg);
       if (write (thisCl->client, tmp, 100) <= 0)
       {
          perror ("[Thread ?]Eroare la write() catre client.\n");
       }
    
       else
          printf ("[Thread ?]Notification sent successfully to [%s].\n", thisCl->ID);
       pthread_mutex_unlock(&mlock);
    }
    else
    {
       printf("Failed to send notification for client with id %s", thisCl->ID);
    } 
}

void cleanup_and_exit(int sig) {
    printf("\nCaught signal %d. Cleaning up...\n", sig);

    if (clientMap) {
        free_hashmap(clientMap, free);
    }
    //if (allStreets) {
        //free_hashmap(allStreets, free);
    //}

    exit(0);
}

void setup_signal_handler() {
    struct sigaction sa;
    sa.sa_handler = cleanup_and_exit;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

void setup_streets()
{
   static int sp0 = 50, 
       sp1 = 10, 
       sp2 = 30, 
       sp3 = 80, 
       sp4 = 120;
       
   insert(allStreets, "Test Street", &sp0);   
   insert(allStreets, "Street 1", &sp1);
   insert(allStreets, "Street 2", &sp2);  
   insert(allStreets, "Street 3", &sp3);
   insert(allStreets, "Street 4", &sp4);
   
   int res = 0;
      int *maxSpeed = (int*)get(allStreets, "Test Street", &res);
        
      
      if(res == 1)
      {
         printf("[Thread ?] found max speed [%d].\n", *maxSpeed);
      }
      else
      {
          printf("[Thread ?] didnt find max speed.\n");
      }  
}

size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct ResponseData *mem = (struct ResponseData *)userp;

    char *ptr = realloc(mem->data, mem->size + realsize + 1);
    if (ptr == NULL) {
        printf("Not enough memory\n");
        return 0;
    }

    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->data[mem->size] = '\0';

    return realsize;
}

void GetWeather(const char * city, char * resTxt)
{
    CURL *curl;
    CURLcode res;

    struct ResponseData response;
    response.data = malloc(1); // Initial buffer allocation
    response.size = 0;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        char api[200]  = "\0";
        sprintf(api, "https://api.tomorrow.io/v4/weather/realtime?location=%s&units=metric&apikey=fvPZnoEC8Ev8yEl9f4XxwTtZ8FMDTeBi", city);
        curl_easy_setopt(curl, CURLOPT_URL, api);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            // Parse JSON response
            cJSON *json = cJSON_Parse(response.data);
            if (json == NULL) {
                fprintf(stderr, "Error parsing JSON\n");
            } else {
                // Navigate to desired fields
                cJSON *data = cJSON_GetObjectItemCaseSensitive(json, "data");
                if (data != NULL) {
                    cJSON *values = cJSON_GetObjectItemCaseSensitive(data, "values");
                    if (values != NULL) {
                        cJSON *precipitationProbability = cJSON_GetObjectItemCaseSensitive(values, "precipitationProbability");
                        cJSON *temperature = cJSON_GetObjectItemCaseSensitive(values, "temperature");
                        cJSON *visibility = cJSON_GetObjectItemCaseSensitive(values, "visibility");
                        
                        if (precipitationProbability != NULL && temperature != NULL) {
                            sprintf(resTxt, "Rain: %.1f%, Temp: %.1fC, Visibility: %.0fkm", precipitationProbability->valuedouble, temperature->valuedouble, visibility->valuedouble);
                        }
                    }
                }
                cJSON_Delete(json); // Clean up
            }
        }

        curl_easy_cleanup(curl);
    }

    free(response.data);
    curl_global_cleanup();
}

void GetPecoID(const char * city, char * resTxt)
{
    CURL *curl;
    CURLcode res;

    struct ResponseData response;
    response.data = malloc(1); // Initial buffer allocation
    response.size = 0;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        char api[200]  = "\0";
        sprintf(api, "https://monitorulpreturilor.info/pmonsvc/Gas/GetUATByName?uatname=%s", city);
        curl_easy_setopt(curl, CURLOPT_URL, api);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
        //printf("GOT PECO ID: %s", response.data);
            // Parse JSON response
            const char *start = strstr(response.data, "<Id>");
    		if (start == NULL) {
        	printf("No <Id> tag found.\n");
        	return;
    		}
    		start += strlen("<Id>");
    	const char *end = strstr(start, "</Id>");
    		if (end == NULL) {
        	printf("No closing </Id> tag found.\n");
        	return 1;
    		}
    		int id_length = end - start;
		char id[50] = "\0";
    		strncpy(id, start, id_length);
    		
    		GetPecoPrices(id, resTxt);
            
        }

        curl_easy_cleanup(curl);
    }

    free(response.data);
    curl_global_cleanup();
}

void GetPecoPrices(const char * id, char * resTxt)
{
    CURL *curl;
    CURLcode res;

    struct ResponseData response;
    response.data = malloc(1); // Initial buffer allocation
    response.size = 0;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        char api[200]  = "\0";
        sprintf(api, "https://monitorulpreturilor.info/pmonsvc/Gas/GetGasItemsByUat?UatId=%s&CSVGasCatalogProductIds=11&OrderBy=dist", id);
        curl_easy_setopt(curl, CURLOPT_URL, api);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
        //printf("GOT PECO ID: %s", response.data);
            // Parse JSON response
            int cnt = 3;
            char name[50] = "\0", price[50] = "\0";
            
            const char *start = response.data; 
            const char *end; 
            while(cnt > 0)
            {
            start = strstr(start, "<Network>");
            if (start == NULL) {
        	printf("No <Network> tag found.\n");
        	return;
    		}
    	    start += strlen("<Network>");
    	    start = strstr(start, "<Id>");
            if (start == NULL) {
        	printf("No <Id> tag found.\n");
        	return;
    		}
    	    start += strlen("<Id>");
    	    
    	    end = strstr(start, "</Id>");
    	    if (end == NULL) {
        	printf("No closing </Id> tag found.\n");
        	return 1;
    		}
    		
    		int id_length = end - start;
    		strncpy(name, start, id_length);
    		
    		
    		start = strstr(start, "<Price>");
            if (start == NULL) {
        	printf("No <Price> tag found.\n");
        	return;
    		}
    		start += strlen("<Price>");
    		end = strstr(start, "</Price>");
    	    if (end == NULL) {
        	printf("No closing </Price> tag found.\n");
        	return 1;
    		}
    		
    		id_length = end - start;
    		strncpy(price, start, id_length);
    		
    		if(strstr(resTxt, name) != NULL) continue;
    		
    		sprintf(resTxt + strlen(resTxt), "%s:%s ", name, price);
    		
            cnt--;
            }
            
        }

        curl_easy_cleanup(curl);
    }

    free(response.data);
    curl_global_cleanup();
}


void GetSport(char * resTxt)
{
    CURL *curl;
    CURLcode res;

    struct ResponseData response;
    response.data = malloc(1); // Initial buffer allocation
    response.size = 0;
    
    curl = curl_easy_init();
    if (curl) {
    char url[200] = "\0";
    sprintf(url, "https://api.sportradar.com/soccer/trial/v4/en/schedules/2024-01-06/summaries.json?start=%d&limit=1&api_key=R51woGUBC6l6kYB5n3MGLUZzd5S5Qsk40Iav2Cxz", rand()%100);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            
            cJSON *json = cJSON_Parse(response.data);
    if (json == NULL) {
        printf("Error parsing JSON\n");
        return;
    }

    cJSON *summaries = cJSON_GetObjectItemCaseSensitive(json, "summaries");
    if (!cJSON_IsArray(summaries)) {
        printf("Summaries not found or not an array\n");
        cJSON_Delete(json);
        return;
    }

    cJSON *match = cJSON_GetArrayItem(summaries, 0); // Fetch the first match summary
    if (match) {
        cJSON *sport_event = cJSON_GetObjectItemCaseSensitive(match, "sport_event");
        cJSON *status = cJSON_GetObjectItemCaseSensitive(match, "sport_event_status");

        if (sport_event && status) {
            cJSON *competitors = cJSON_GetObjectItemCaseSensitive(sport_event, "competitors");
            cJSON *home_score = cJSON_GetObjectItemCaseSensitive(status, "home_score");
            cJSON *away_score = cJSON_GetObjectItemCaseSensitive(status, "away_score");
            cJSON *match_status = cJSON_GetObjectItemCaseSensitive(status, "status");

            if (cJSON_IsArray(competitors) && cJSON_GetArraySize(competitors) == 2) {
                cJSON *home_team = cJSON_GetArrayItem(competitors, 0);
                cJSON *away_team = cJSON_GetArrayItem(competitors, 1);

                const char *home_abbreviation = cJSON_GetObjectItemCaseSensitive(home_team, "abbreviation")->valuestring;
                const char *away_abbreviation = cJSON_GetObjectItemCaseSensitive(away_team, "abbreviation")->valuestring;
                
		const char *home_country = cJSON_GetObjectItemCaseSensitive(home_team, "country_code")->valuestring;
                const char *away_country = cJSON_GetObjectItemCaseSensitive(away_team, "country_code")->valuestring;
                
                if (home_score && away_score && strcmp(match_status->valuestring, "closed") == 0) {
                    sprintf(resTxt, "(%s) %s %d-%d %s (%s)",home_country, home_abbreviation, home_score->valueint, away_score->valueint, away_abbreviation, away_country);
                } else if (strcmp(match_status->valuestring, "not_started") == 0) {
                    sprintf(resTxt, "(%s) %s ?-? %s (%s)",home_country, home_abbreviation, away_abbreviation, away_country);
                } else {
                    sprintf(resTxt,"(%s) %s ?-? %s (%s)",home_country, home_abbreviation, away_abbreviation, away_country);
                }
            }
        } else {
            printf("Invalid match data\n");
        }
    } else {
        printf("No match found for the given index.\n");
    }

    cJSON_Delete(json);
        }

        curl_easy_cleanup(curl);
        
    }
    free(response.data);
    curl_global_cleanup();
}

int InitializeDB()
{
   sqlite3 *db;
   int rc;
   char *err_msg = 0;
   
   rc = sqlite3_open("test.db", &db);
   
   if(rc)
   {
      fprintf(stderr, "Cannout open databse: %s\n", sqlite3_errmsg(db));
      return -1;  
   }
   else printf("open ok \n");
   const char* sql = "create table if not exists users(name text, pass text, pref text);";
   
   rc = sqlite3_exec(db, sql, 0, 0, &err_msg);

   return 0;
}

int RetreiveOrAddUser(const char *name, const char* pass, char* pref, int changePref)
{
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc;
    const char* default_pref = "null";

    rc = sqlite3_open("test.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    const char *check_query = "SELECT name, pass, pref FROM users WHERE name = ? AND pass = ?;";
    rc = sqlite3_prepare_v2(db, check_query, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -1;
    }


    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, pass, -1, SQLITE_STATIC);


    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *ret_name = sqlite3_column_text(stmt, 0);
        const unsigned char *ret_pass = sqlite3_column_text(stmt, 1);
        const unsigned char *ret_pref = sqlite3_column_text(stmt, 2);

        printf("User found:\n");
        printf("Name: %s, Pass: %s, Pref: %s\n", ret_name, ret_pass, ret_pref);
        
        if (changePref == 1) {
            sqlite3_finalize(stmt); // Finalize the SELECT statement
            const char *update_query = "UPDATE users SET pref = ? WHERE name = ? AND pass = ?;";
            rc = sqlite3_prepare_v2(db, update_query, -1, &stmt, 0);
            if (rc != SQLITE_OK) {
                fprintf(stderr, "Failed to prepare update statement: %s\n", sqlite3_errmsg(db));
                sqlite3_close(db);
                return -1;
            }
        // Bind parameters (new pref, name, pass)
            sqlite3_bind_text(stmt, 1, pref, -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, name, -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 3, pass, -1, SQLITE_STATIC);

            rc = sqlite3_step(stmt);
            if (rc == SQLITE_DONE) {
                printf("Preferences updated to: %s\n", pref);
            } else {
                fprintf(stderr, "Failed to update preferences: %s\n", sqlite3_errmsg(db));
            }
            strcpy(pref, pref);
        }
        else
        {
         strcpy(pref, ret_pref);
        }
        
	
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1; // found and retreived
    }
    sqlite3_finalize(stmt);

    const char *insert_query = "INSERT INTO users (name, pass, pref) VALUES (?, ?, ?);";
    rc = sqlite3_prepare_v2(db, insert_query, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare insert statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -1;
    }

    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, pass, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, default_pref, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed to insert user: %s\n", sqlite3_errmsg(db));
    } else {
        printf("New user added: Name: %s, Pass: %s, Pref: %s\n", name, pass, default_pref);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return 0;
}


