#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

#define MAX_BOOK 10
#define MAX_TABLE 5
#define MAX_CUSTOMER 200

// Data structures
typedef struct{
    char Bookname[200];
    int Bookstatus; // 1: Available, 0: Borrowed
} Book;

typedef struct{
    int Tablenumber;
    int Tablestatus; // 1: Available, 0: Occupied
} Table;

typedef struct{
    int table;
    int books[3];
    int book_count;
} Customer;

// Global variables
Table tables[MAX_TABLE];
Book book_inventory[MAX_BOOK];
Customer customers[MAX_CUSTOMER];
int total_tables = 0, total_books = 0, total_customers = 0;

// Synchronization tools
sem_t table_semaphore;
sem_t customer_semaphores[MAX_CUSTOMER];
pthread_mutex_t table_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t book_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t customer_mutex = PTHREAD_MUTEX_INITIALIZER;

// Function prototypes
void initialize_resources();
void load_books();
void load_tables();
void handle_customer(int id);
void *customer_thread(void *arg);
void show_menu();

// Initialize resources
void initialize_resources(){
    sem_init(&table_semaphore, 0, 1);
    for (int i = 0; i < MAX_CUSTOMER; i++)
    {
        sem_init(&customer_semaphores[i], 0, 0);
    }
}

// Load book inventory
void load_books(){
    for (int i = 0; i < MAX_BOOK; i++){
        snprintf(book_inventory[i].Bookname, sizeof(book_inventory[i].Bookname), "Book %d", i + 1);
        book_inventory[i].Bookstatus = 1;
        total_books++;
    }
    printf("\n\n**************************************");
    printf("\n* %d books loaded into the inventory *\n", total_books);
}

// Load tables
void load_tables(){
    for (int i = 0; i < MAX_TABLE; i++){
        tables[i].Tablenumber = i + 1;
        tables[i].Tablestatus = 1;
        total_tables++;
    }
    printf("*====================================*\n");
    printf("*   %d tables loaded and available    *\n", total_tables);
    printf("**************************************\n");
}

// Customer handling
void handle_customer(int id){
    printf("Customer %d searching for a table...\n", id);
    while (1){
        sem_wait(&table_semaphore);
        pthread_mutex_lock(&table_mutex);

        int table_found = 0;
        for (int i = 0; i < total_tables; i++){
            if (tables[i].Tablestatus == 1)
            {
                tables[i].Tablestatus = 0;
                customers[id].table = tables[i].Tablenumber;
                customers[id].book_count = 0;
                total_customers++;
                table_found = 1;
                printf("Customer %d assigned to table %d.\n", id, tables[i].Tablenumber);
                break;
            }
        }

        pthread_mutex_unlock(&table_mutex);
        sem_post(&table_semaphore);

        if (table_found)
            break;

        printf("#----------------------------------------#\n");
        printf("No tables available. Customer %d waiting...\n", id);
        sleep(1);
    }

    pthread_mutex_lock(&book_mutex);
    int books_picked = 0;
    for (int i = 0; i < total_books && books_picked < 3; i++){
        if (book_inventory[i].Bookstatus == 1)
        {
            book_inventory[i].Bookstatus = 0;
            customers[id].books[books_picked++] = i;
        }
    }
    customers[id].book_count = books_picked;
    pthread_mutex_unlock(&book_mutex);

    printf("Customer %d borrowed %d books and is seated at table %d.\n", id, books_picked, customers[id].table);
    sleep(2);

    pthread_mutex_lock(&table_mutex);
    tables[customers[id].table - 1].Tablestatus = 1;
    pthread_mutex_unlock(&table_mutex);

    pthread_mutex_lock(&book_mutex);
    for (int i = 0; i < books_picked; i++)
    {
        book_inventory[customers[id].books[i]].Bookstatus = 1;
    }
    pthread_mutex_unlock(&book_mutex);

    printf("Customer %d has left table %d.\n", id, customers[id].table);
}

// Customer thread function
void *customer_thread(void *arg){
    int id = *(int *)arg;
    free(arg);
    handle_customer(id);
    return NULL;
}

// Show menu
void show_menu(){
    int choice;
    while (1)
    {
        printf("\n|******| Welcome To W&L's Café |******|\n");
        printf("\n**************************************\n");
        printf("*********  Chosse Option :-  *********\n");
        printf("*********  1. Add customers  *********\n*********  2. Exit           *********\n");
        printf("**************************************\n");
        printf("\nEnter your choice : ");
        scanf("%d", &choice);
        switch (choice)
        {
        case 1:
        {
            int n;
            printf("Simulation will be started\n");
            printf("Enter number of customers : ");
            scanf("%d", &n);
            pthread_t threads[n];
            for (int i = 0; i < n; i++){
                int *id = malloc(sizeof(int));
                *id = i + total_customers;
                pthread_create(&threads[i], NULL, customer_thread, id);
            }
            for (int i = 0; i < n; i++)
            {
                pthread_join(threads[i], NULL);
            }
            break;
        }
        case 2:
            printf("\n*****************************\n");
            printf("* %2d Customer Visited Today *\n", total_customers);
            printf("** Cafe Closed For Today ! **");
            printf("\n*****************************\n\n");
            return;
        default:
            printf("Invalid choice. Please try again.\n");
        }
    }
}

int main(){
    srand(time(NULL));
    initialize_resources();
    load_books();
    load_tables();
    show_menu();
    return 0;
}