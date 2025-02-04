#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <gtk/gtk.h>

extern int errno;
int SD;

  GtkApplication *app;  
  GtkWidget *grid;
  GtkWidget *username_label, *password_label, *city_label,*login_button;
    
  GtkWidget *login_window;
  GtkWidget *main_window;

  GtkWidget *window;
  GtkWidget *mainG, *top, *notif, *notif_accident;
  GtkWidget *b_accident;
  GtkWidget *box_accident, *box_notif, *box_notif_accident;
  GtkWidget *t_speed, *t_street;
  
  GtkWidget *b_weather, *b_gas, *b_sport, *t_weather, *t_gas, *t_sport;
  GtkWidget *box_preferences, *box_prefInfo;
   
  GtkCssProvider *css_provider;

static void activate (GtkApplication* app, gpointer user_data);
static void show_main_app (GtkApplication* app);
static void on_login_button_clicked(GtkButton *button, gpointer user_data);

static void ManageWeatherPreferences (GtkWidget *widget, gpointer data);
static void ManageGasPreferences (GtkWidget *widget, gpointer data);
static void ManageSportPreferences (GtkWidget *widget, gpointer data);
static void RefactorPreferences();
void UpdateSpeed(int newSpeed);
void UpdateStreet(const char * newStreet);

int port;
char* makeID(int digits);

void *GetInput(void * sd);
void *SendUpdates(void * sd);
void *ReceiveUpdates(void * sd);

int Speed = 80;
char Street[100] = "Street 3";
char ID[100] = "";
char Flags[100] = "WeatherGas";
bool weather, gas, sport;
char USERNAME[100], PASSWORD[100], CITY[100];
int argC;
char **argV;

gboolean my_background_task(gpointer user_data) {


    return FALSE;
}

typedef struct {
    GtkWidget *username_entry;
    GtkWidget *password_entry;
    GtkWidget *city_entry;
} LoginWidgets;


static void report_accident (GtkWidget *widget, gpointer data)
{
  char tmp[100];
    sprintf(tmp, "%s-%s-", ID, "accident");
      
      if (write (SD, tmp, 100) <= 0)
      {
         perror ("[client]Eroare la write() spre server.\n");
      }
}

int main (int argc, char *argv[])
{
  srand(time(NULL));
  
  if (argc != 3)
  {
     printf ("[client] Sintaxa: %s <adresa_server> <port>\n", argv[0]);
     return -1;
  }
  
  argC = argc;

    // Allocate memory for the array of string pointers
    argV = malloc(argc * sizeof(char *));
    if (argV == NULL) {
        perror("Failed to allocate memory for argV");
        exit(EXIT_FAILURE);
    }

    // Copy each string in argv
    for (int i = 0; i < argc; i++) {
        argV[i] = strdup(argv[i]); // Allocate and copy each string
        if (argV[i] == NULL) {
            perror("Failed to allocate memory for argument string");
            exit(EXIT_FAILURE);
        }
    }
    
  int status;
  app = gtk_application_new ("com.app.Trafficy",  G_APPLICATION_NON_UNIQUE);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run (G_APPLICATION (app), NULL, NULL);   
  g_object_unref (app);
  
}

int main2(int argc, char *argv[])
{
  int sd = 0, rd = 0;			
  struct sockaddr_in server;	
  char msg[1024] = "\0", ch;		

  printf("Started main2 with argc: %d and argv %s %s\n", argc, argv[1], argv[2]);

  port = atoi (argv[2]);

  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
  {
     perror ("[client] Eroare la socket().\n");
     return errno;
  }
  
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = inet_addr(argv[1]);
  server.sin_port = htons (port);
  
  if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)
  {
     perror ("[client]Eroare la connect().\n");
     return errno;
  }

   SD = sd;
   printf("descriptor: %d\n", sd);
   fflush(stdout);
   /*pthread_t input_thread;
   if(pthread_create(&input_thread, NULL, &GetInput, &sd) != 0)
   {
      perror("Failed input thread");
      return -1;
   }*/
   
   pthread_t update_thread;
   if(pthread_create(&update_thread, NULL, &SendUpdates, (void *)(intptr_t)sd) != 0)
   {
      perror("Failed input thread");
      return -1;
   }
   
   pthread_t receive_thread;
   if(pthread_create(&receive_thread, NULL, &ReceiveUpdates, (void *)(intptr_t)sd) != 0)
   {
      perror("Failed input thread");
      return -1;
   }
   //printf("descriptor2: %d\n", &sd);
}

void *ReceiveUpdates(void * sd)
{
char msg[200];
char ch;
int rd;
int SD = (int)(intptr_t)sd;
//printf("receive sd: %d", SD);
for(;;)
   {
      bzero(msg, sizeof(msg));
      
      while(1)
      {
         rd = read(SD, &ch, 1);
         
         if (rd < 0)
         {
            printf ("[client]Eroare la read() de la server %d %d.\n", rd, SD);
            return;
         } else if (rd == 0) continue;
         if(ch != '|')
            strncat(msg, &ch, 1);
         else
            break;
            
      }
      
      
      printf("Message received: [%s].\n", msg);
      if(strncmp(msg, "[ATTENTION]", strlen("[ATTENTION]")) == 0)
      {
         gtk_label_set_text(notif, msg + strlen("[ATTENTION]"));
      }
      if(strncmp(msg, "{W}", strlen("{W}")) == 0)
      {
         gtk_label_set_text(t_weather, msg + strlen("{W}"));
      }
      if(strncmp(msg, "{G}", strlen("{G}")) == 0)
      {
         gtk_label_set_text(t_gas, msg + strlen("{G}"));
      }
      if(strncmp(msg, "{S}", strlen("{S}")) == 0)
      {
         gtk_label_set_text(t_sport, msg + strlen("{S}"));
      }
      if(strncmp(msg, "[A]", strlen("[A]")) == 0)
      {
         gtk_label_set_text(notif_accident, msg + strlen("[A]"));
      }
      if(strncmp(msg, "[setFlags]", strlen("[setFlags]")) == 0)
      {
         strcpy(Flags, msg + strlen("[setFlags]"));
         printf("Set flags from db: %s\n", Flags);
         
         weather = strstr(Flags, "Weather") == NULL;
         gas = strstr(Flags, "Gas") == NULL;
         sport = strstr(Flags, "Sport") == NULL;
         
         ManageWeatherPreferences(b_weather, NULL);
         ManageGasPreferences(b_gas, NULL);
         ManageSportPreferences(b_sport, NULL);
         
      }
      
      fflush(stdout);
      
   }
}

void *SendUpdates(void * sd)
{
int wr;
   int SD = (int)(intptr_t)sd;
printf("receive sd: %d", SD);
   char msg[100] = "";
   sleep(1);
     
   bzero (msg, sizeof(msg));
   strcpy(ID, makeID(10));
  
   sprintf(msg, "%s-setup-%s-%s-%s-", ID, USERNAME, PASSWORD, CITY);
  
   printf ("[client]Process started with id: %s\n", ID);
   fflush (stdout);
   wr = write (SD, msg, strlen(msg));
   if (wr <= 0)
   {
      perror ("[client]Eroare la write() spre server.\n");
      return errno;
   }
   sleep(1);
   
   for(;;)
   {
      
      bzero(msg, 100);
      Speed = rand() % 151;
      UpdateSpeed(Speed);
      sprintf(msg, "%s-info-%s-%d-%s-%s-", ID, Street, Speed, Flags, CITY);
      
      if (write (SD, msg, 100) < 0)
      {
         perror ("[client]Eroare la write() spre server.\n");
         break;
      }
      sleep(30);
      
      
   }
}

void *GetInput(void * sd)
{
   int SD = *(int*)sd;
   char inp[100], tmp[100] ="";
   bzero(inp, 100);
   
   while(read(0, inp, 100))
   {
      inp[strcspn(inp, "\n")] = '\0';
      
      
      sprintf(tmp, "%s-%s-", ID, inp);
      printf("Got input [%s]\n", inp);
      
      bzero(inp, 100);
      
      if (write (SD, tmp, 100) < 0)
      {
         perror ("[client]Eroare la write() spre server.\n");
         break;
      }
      
   }
}


char *makeID(int digits) {
    if (digits <= 0 || digits >= 100) {
        return NULL;
    }

    static char id[100] = {0};

    //srand((unsigned int)time(NULL));

    for (int i = 0; i < digits; i++) {
        id[i] = '0' + (rand() % 10);
    }

    id[digits] = '\0';

    return id;
}


static void activate(GtkApplication *app, gpointer user_data) {
    
    
    LoginWidgets *widgets = g_new(LoginWidgets, 1);

    // window
    login_window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(login_window), "Login Screen");
    gtk_window_set_default_size(GTK_WINDOW(login_window), 400, 711);
    gtk_window_set_resizable(GTK_WINDOW(login_window), false);

    // grid
    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_window_set_child(GTK_WINDOW(login_window), grid);

    // username
    username_label = gtk_label_new("Username:");
    gtk_grid_attach(GTK_GRID(grid), username_label, 0, 0, 1, 1);

    widgets->username_entry = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), widgets->username_entry, 1, 0, 1, 1);

    // password
    password_label = gtk_label_new("Password:");
    gtk_grid_attach(GTK_GRID(grid), password_label, 0, 1, 1, 1);
    
    widgets->password_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(widgets->password_entry), FALSE); 
    gtk_grid_attach(GTK_GRID(grid), widgets->password_entry, 1, 1, 1, 1);
    
    // city
    city_label = gtk_label_new("City:");
    gtk_grid_attach(GTK_GRID(grid), city_label, 0, 2, 1, 1);
    
    widgets->city_entry = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), widgets->city_entry, 1, 2, 1, 1);
    

    // Clogin but
    login_button = gtk_button_new_with_label("Login");
    gtk_grid_attach(GTK_GRID(grid), login_button, 1, 3, 1, 1);

    // onClick
    g_signal_connect(login_button, "clicked", G_CALLBACK(on_login_button_clicked), widgets);

    gtk_window_present(GTK_WINDOW(login_window));
}
  
static void on_login_button_clicked(GtkButton *button, gpointer user_data) {

    LoginWidgets *widgets = (LoginWidgets *)user_data;

    // Access username and password entries
    const char *username = gtk_editable_get_text(GTK_EDITABLE(widgets->username_entry));
    const char *password = gtk_editable_get_text(GTK_EDITABLE(widgets->password_entry));
    const char *city = gtk_editable_get_text(GTK_EDITABLE(widgets->city_entry));

    //g_print("Username: %s\n", username);
    //g_print("Password: %s\n", password);
    
    strcpy(USERNAME, username);
    strcpy(PASSWORD, password);
    strcpy(CITY, city);
    
    
    // Show the main application window
    gtk_window_destroy(login_window);
    show_main_app(app);
}

static void show_main_app (GtkApplication* app)
{
  
  window = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (window), "Trafficy");
  gtk_window_set_default_size (GTK_WINDOW (window), 400, 711);
  gtk_window_set_resizable(GTK_WINDOW(window), false);
  
  //top
  top = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
  gtk_widget_set_hexpand(top, TRUE); 
  
  //main
  mainG = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_widget_set_vexpand(mainG, TRUE);
  gtk_widget_set_hexpand(mainG, TRUE); 
  gtk_window_set_child (GTK_WINDOW (window), mainG);
  
  css_provider = gtk_css_provider_new();
  gtk_css_provider_load_from_data(css_provider,
                                 "#main_container { background-color: white; }", -1);

  gtk_widget_set_name(mainG, "main_container");

  GtkStyleContext *context = gtk_widget_get_style_context(mainG);
  gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
  
  //preferences
  box_preferences = gtk_box_new (GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_halign (box_preferences, GTK_ALIGN_START);
  gtk_widget_set_valign (box_preferences, GTK_ALIGN_CENTER);
  gtk_widget_set_margin_top(box_preferences, 10);
  
  box_prefInfo = gtk_box_new (GTK_ORIENTATION_VERTICAL, 25);
  gtk_widget_set_halign (box_prefInfo, GTK_ALIGN_START);
  gtk_widget_set_valign (box_prefInfo, GTK_ALIGN_CENTER);
  gtk_widget_set_margin_top(box_prefInfo, 10);
  
  gtk_box_append(GTK_BOX(top), box_preferences);
  gtk_box_append(GTK_BOX(top), box_prefInfo);
  
  //weather
  b_weather = gtk_button_new_with_label ("Weather");
  g_signal_connect (b_weather, "clicked", G_CALLBACK (ManageWeatherPreferences), NULL);
  gtk_box_append (GTK_BOX (box_preferences), b_weather);
  weather = !weather;  
  
  t_weather = gtk_label_new("-");
  gtk_label_set_xalign(GTK_LABEL(t_weather), 0.0);
  gtk_box_append(GTK_BOX(box_prefInfo), t_weather);
  
  //gas  
  b_gas = gtk_button_new_with_label ("Gas");
  g_signal_connect (b_gas, "clicked", G_CALLBACK (ManageGasPreferences), NULL);
  gtk_box_append (GTK_BOX (box_preferences), b_gas);
  gas = !gas;
  
  
  t_gas = gtk_label_new("-");
  gtk_label_set_xalign(GTK_LABEL(t_gas), 0.0);
  gtk_box_append(GTK_BOX(box_prefInfo), t_gas);
  
  //sport 
  b_sport = gtk_button_new_with_label ("Sport");
  g_signal_connect (b_sport, "clicked", G_CALLBACK (ManageSportPreferences), NULL);
  gtk_box_append (GTK_BOX (box_preferences), b_sport);
  sport = !sport;   
  
  t_sport = gtk_label_new("-");
  gtk_label_set_xalign(GTK_LABEL(t_sport), 0.0);
  gtk_box_append(GTK_BOX(box_prefInfo), t_sport);
  
  ManageWeatherPreferences(b_weather, NULL);
  ManageGasPreferences(b_gas, NULL);
  ManageSportPreferences(b_sport, NULL);
  
  
  gtk_box_append(GTK_BOX(mainG), top);
  
  //speed
  t_speed = gtk_label_new("-");
  gtk_label_set_xalign(GTK_LABEL(t_speed), 0.0);
  gtk_box_append(GTK_BOX(mainG), t_speed);
  UpdateSpeed(Speed);
  
  //street
  t_street = gtk_label_new("-");
  gtk_label_set_xalign(GTK_LABEL(t_street), 0.0);
  gtk_box_append(GTK_BOX(mainG), t_street);
  UpdateStreet(Street);
  
  //notif
  box_notif = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_halign (box_notif, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (box_notif, GTK_ALIGN_CENTER);
  gtk_widget_set_margin_top(box_notif, 10);
  
  notif = gtk_label_new("");
  //gtk_label_set_wrap(GTK_LABEL(notif), TRUE);
  //gtk_label_set_wrap_mode(GTK_LABEL(notif), PANGO_WRAP_WORD); // Wrap by word
  gtk_widget_set_size_request(notif, 100, -1);
  gtk_label_set_ellipsize(GTK_LABEL(notif), PANGO_ELLIPSIZE_END);
  
  gtk_box_append(GTK_BOX(box_notif), notif);
  gtk_box_append(GTK_BOX(mainG), box_notif);
  
  //accident notif
  box_notif_accident = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_halign (box_notif_accident, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (box_notif_accident, GTK_ALIGN_CENTER);
  gtk_widget_set_margin_top(box_notif_accident, 10);
  
  notif_accident = gtk_label_new("");
  //gtk_label_set_wrap(GTK_LABEL(notif), TRUE);
  //gtk_label_set_wrap_mode(GTK_LABEL(notif), PANGO_WRAP_WORD); // Wrap by word
  gtk_widget_set_size_request(notif_accident, 100, -1);
  gtk_label_set_ellipsize(GTK_LABEL(notif_accident), PANGO_ELLIPSIZE_END);
  
  gtk_box_append(GTK_BOX(box_notif_accident), notif_accident);
  gtk_box_append(GTK_BOX(mainG), box_notif_accident);
  
  
  
  //accident report
  box_accident = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_halign (box_accident, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (box_accident, GTK_ALIGN_START);
  gtk_widget_set_margin_top(box_accident, 350);

  b_accident = gtk_button_new_with_label ("Report Accident");
  g_signal_connect (b_accident, "clicked", G_CALLBACK (report_accident), NULL);
  gtk_box_append (GTK_BOX (box_accident), b_accident);
  gtk_box_append(GTK_BOX(mainG), box_accident);
  
  gtk_window_present (GTK_WINDOW (window)); 
  g_idle_add(my_background_task, NULL);
  
  main2(argC, argV);
}

static void ManageWeatherPreferences (GtkWidget *widget, gpointer data)
{
weather = !weather;
RefactorPreferences();

  GtkCssProvider *css_provider = gtk_css_provider_new();
  if(weather)
  gtk_css_provider_load_from_data(css_provider, "#custom { background-color: green; }", -1);
  else
  { gtk_css_provider_load_from_data(css_provider, "#custom { background-color: red; }", -1); gtk_label_set_text(t_weather, "-"); }

  gtk_widget_set_name(widget, "custom");

  GtkStyleContext *context = gtk_widget_get_style_context(widget);
  gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
  g_object_unref(css_provider);
}
static void ManageGasPreferences (GtkWidget *widget, gpointer data)
{
gas = !gas;
RefactorPreferences();

GtkCssProvider *css_provider = gtk_css_provider_new();
  if(gas)
  gtk_css_provider_load_from_data(css_provider, "#custom { background-color: green; }", -1);
  else
  { gtk_css_provider_load_from_data(css_provider, "#custom { background-color: red; }", -1); gtk_label_set_text(t_gas, "-"); }

  gtk_widget_set_name(widget, "custom");

  GtkStyleContext *context = gtk_widget_get_style_context(widget);
  gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
  g_object_unref(css_provider);
}
static void ManageSportPreferences (GtkWidget *widget, gpointer data)
{
sport = !sport;
RefactorPreferences();

GtkCssProvider *css_provider = gtk_css_provider_new();
  if(sport)
  gtk_css_provider_load_from_data(css_provider, "#custom { background-color: green; }", -1);
  else
  { gtk_css_provider_load_from_data(css_provider, "#custom { background-color: red; }", -1); gtk_label_set_text(t_sport, "-"); }

  gtk_widget_set_name(widget, "custom");

  GtkStyleContext *context = gtk_widget_get_style_context(widget);
  gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
  g_object_unref(css_provider);
}

static void RefactorPreferences()
{
   strcpy(Flags, "\0");
   sprintf(Flags, "%s%s%s", weather ? "Weather" : "", gas ? "Gas" : "", sport ? "Sport" : "");
   
   
   printf("New flags: %s.\n", Flags);
   fflush(stdout);
}

void UpdateSpeed(int newSpeed)
{
   Speed = newSpeed;
   char sp[10];
   sprintf(sp, "%d km/h", Speed);
   
   gtk_label_set_text(t_speed, sp);
   
   GtkCssProvider *css_provider = gtk_css_provider_new();
   
   gtk_css_provider_load_from_data(css_provider, "#custom { font-size: 20px; }", -1);
   gtk_widget_set_name(t_speed, "custom");
   
   GtkStyleContext *context = gtk_widget_get_style_context(t_speed);
   gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
   g_object_unref(css_provider);
}

void UpdateStreet(const char * newStreet)
{
   strcpy(Street, newStreet);
    char sp[100];
   sprintf(sp, "St: %s", Street);
   
   gtk_label_set_text(t_street, sp);
   
   GtkCssProvider *css_provider = gtk_css_provider_new();
   
   gtk_css_provider_load_from_data(css_provider, "#custom { font-size: 20px; }", -1);
   gtk_widget_set_name(t_street, "custom");
   
   GtkStyleContext *context = gtk_widget_get_style_context(t_street);
   gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
   g_object_unref(css_provider);
}

