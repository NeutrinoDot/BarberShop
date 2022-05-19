// Matthew Kim CSS503 A
// Modified: 08/05/22
// ------------------------------------------
// Implements a barber shop with multiple barbers who provide service to 
// customers. Creates multiple barber and customer threads to process service
// interactions at the barber shop. Prints the barber shop activities to 
// console.
// --------------------------------------------------------------

#include <iostream>
#include <sys/time.h>
#include <unistd.h>
#include "Shop.h"
using namespace std;

void *barber(void *);
void *customer(void *);

// ThreadParam class
// A set of parameters to be passed to each thread. This class is used as a way
// to pass more than one argument to a thread. 
class ThreadParam {
public:
  ThreadParam(Shop* shop, int id, int service_time) :
    shop(shop), id(id), service_time(service_time) {
  };
  Shop* shop; // a pointer to the Shop object
  int id; //thread identifier
  int service_time; //service time (usec) for barber.
};

int main(int argc, char *argv[]) {
  // Read arguments from command line
  if (argc != 5) {
    cout << "Usage: num_barbers num_chairs num_customers service_time" << endl;
    return -1;
  }

  // argv[0] is the program name.
  int num_barbers = atoi(argv[1]);
  int num_chairs = atoi(argv[2]);
  int num_customers = atoi(argv[3]);
  int service_time = atoi(argv[4]);

  // many barbers, many customers, one shop
  pthread_t barber_thread[num_barbers];
  pthread_t customer_threads[num_customers];
  Shop shop(num_barbers, num_chairs);

  // spawns num_barbers number of barber threads. Each thread gets passed a
  // pointer to the shop object, the barber id, and service time.
  for (int i = 0; i < num_barbers; i++) {
    int barberId = i; //barberId starts from 0
    ThreadParam* barber_param = new ThreadParam(&shop, barberId, service_time);
    pthread_create(&barber_thread[i], NULL, barber, barber_param);
  }

  // loops spawning num_customers, waits a random interval time between each new
  // customer being spawned. All customer threads are passed in a pointer to the
  // shop object and the customer identifier.
  for (int i = 0; i < num_customers; i++) {
    usleep(rand() % 1000);
    int custId = i + 1;
    ThreadParam* customer_param = new ThreadParam(&shop, custId, 0);
    pthread_create(&customer_threads[i], NULL, customer, customer_param);
  }

  // Wait for all customers threads to terminate
  for (int i = 0; i < num_customers; i++) {
    pthread_join(customer_threads[i], NULL);
  }

  // terminates all barber threads
  for (int i = 0; i < num_barbers; i++) {
    pthread_cancel(barber_thread[i]);
  }

  cout << "# customers who didn't receive a service = " << shop.get_cust_drops() << endl;
  return 0;
}

// function run by each barber thread
void *barber(void *arg) {
  // extract parameters
  ThreadParam* barber_param = (ThreadParam*) arg;
  Shop& shop = *barber_param->shop;
  int barberId = barber_param->id;
  int service_time = barber_param->service_time;
  delete barber_param;

  // barber keeps looping through accepting customer, providing service, and 
  // releasing customer until being terminated by the main
  while(true) {
    shop.helloCustomer(barberId); //pick up a new customer
    usleep(service_time);
    shop.byeCustomer(barberId); //release the customer
  }
  return nullptr;
}

// function run by each customer thread
void *customer(void *arg) {
  ThreadParam* customer_param = (ThreadParam*) arg;
  Shop& shop = *customer_param->shop;
  int custId = customer_param->id;
  delete customer_param;
  
  // customer visits shop. Customer leaves shop if no barbers are available.
  int barberId = -1;
  if ((barberId = shop.visitShop(custId)) != -1) {
    shop.leaveShop(custId, barberId); // wait until customer service is finished
  }
  return nullptr;
}
