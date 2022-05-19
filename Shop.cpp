// Matthew Kim CSS503 A
// Modified: 08/05/22
// --------------------------------------------------------------
// Implementation of the Shop class.
// --------------------------------------------------------------

#include "Shop.h"

// Destructor. Frees all dynamically allocated member variables.
Shop::~Shop() {
  delete[] customer_in_chair;
  customer_in_chair = nullptr;
  delete[] in_service;
  in_service = nullptr;
  delete[] money_paid;
  money_paid = nullptr;
  for (int i = 0; i < num_barbers; i++) {
    pthread_cond_destroy(&cond_customer_served[i]);
    pthread_cond_destroy(&cond_barber_paid[i]);
    pthread_cond_destroy(&cond_barber_sleeping[i]);
  }
  delete[] cond_customer_served;
  cond_customer_served = nullptr;
  delete[] cond_barber_paid;
  cond_barber_paid = nullptr;
  delete[] cond_barber_sleeping;
  cond_barber_sleeping = nullptr;
}

// Helper to the constructors. Initializes member variables.
void Shop::init() {
  customer_in_chair = new int[num_barbers];
  in_service = new bool[num_barbers];
  money_paid = new bool[num_barbers];
  cond_customer_served = new pthread_cond_t[num_barbers];
  cond_barber_paid = new pthread_cond_t[num_barbers];
  cond_barber_sleeping = new pthread_cond_t[num_barbers];
  for (int i = 0; i < num_barbers; i++) {
    customer_in_chair[i] = 0;
    in_service[i] = false;
    money_paid[i] = false;
    pthread_cond_init(&cond_customer_served[i], NULL);
    pthread_cond_init(&cond_barber_paid[i], NULL);
    pthread_cond_init(&cond_barber_sleeping[i], NULL);
  }

  cust_drops = 0;
  wait_count = 0;
  in_chair_count = 0;
  pthread_mutex_init(&mMutex, NULL);
  pthread_cond_init(&cond_customers_waiting, NULL);
}

// Converts int to string.
string Shop::int2string(int i) {
  stringstream out;
  out << i;
  return out.str();
}

// Prints a message to console.
void Shop::print(bool isCustomer, int person, string message) {
  cout << (isCustomer ? "customer[" : "barber  [") << person << "]: " 
    << message << endl;
}

// Returns the total number of customers not serviced by barbers.
int Shop::get_cust_drops() const {
  return cust_drops;
}

// Processes a customer who visits the barber shop. Returns the barberId of the
// barber who serviced the customer. Returns -1 if no one serviced the
// customer.
int Shop::visitShop(int custId) {
  pthread_mutex_lock(&mMutex);
   
  // If all chairs are full then leave shop
  if (in_chair_count == num_barbers  && wait_count == num_chairs) {
    print(true, custId, 
      "leaves the shop because of no available waiting chairs.");
    ++cust_drops;
    pthread_mutex_unlock(&mMutex);
    return -1;
  }
  
  // If all barbers are busy or there is already a customer waiting
  // then take a chair and wait for service
  if (in_chair_count == num_barbers || wait_count != 0) {
    wait_count++;
    print(true, custId, "takes a waiting chair. # waiting seats available = " + 
      int2string(num_chairs - wait_count));
    pthread_cond_wait(&cond_customers_waiting, &mMutex);
    wait_count--;
  }
  
  // select barberId for the customer
  int barberId = -1;
  for (int i = 0; i < num_barbers; i++) {
    if (customer_in_chair[i] == 0) {
      barberId = i;
      break;
    }
  }
  if (barberId == -1) {
    cout << "ERROR: No barbers available for customer[" << custId << "]" 
      << endl;
  }

  print(true, custId, "moves to a service chair[" + int2string(barberId) + 
    "]. # waiting seats available = " + int2string(num_chairs - wait_count));
  customer_in_chair[barberId] = custId;
  in_chair_count++;
  in_service[barberId] = true;
  
  // wake up the barber just in case if he is sleeping
  pthread_cond_signal(&cond_barber_sleeping[barberId]);

  pthread_mutex_unlock(&mMutex); 
  return barberId;
}

// Processes a customer who is serviced by a barber and leaves after the service.
void Shop::leaveShop(int custId, int barberId) {
  pthread_mutex_lock(&mMutex);
  
  // Wait for service to be completed
  print(true, custId, "wait for barber[" + int2string(barberId) + 
    "] to be done with hair-cut");
  while (in_service[barberId] == true) {
    pthread_cond_wait(&cond_customer_served[barberId], &mMutex);
  }
  
  // Pay the barber and signal barber appropriately
  money_paid[barberId] = true;
  pthread_cond_signal(&cond_barber_paid[barberId]);
  print(true, custId, "says good-bye to the barber[" + 
    int2string(barberId) + "]");
  
  pthread_mutex_unlock(&mMutex);
}

// Processes a barber who is available to service the next customer.
void Shop::helloCustomer(int barberId) {
  pthread_mutex_lock(&mMutex);
  
  // If no customers than barber can sleep
  if (wait_count == 0 && customer_in_chair[barberId] == 0 ) {
    print(false, barberId, "sleeps because of no customers.");
    pthread_cond_wait(&cond_barber_sleeping[barberId], &mMutex);
  }

  // check if the customer sits down at the barber's seat
  if (customer_in_chair[barberId] == 0) {              
    pthread_cond_wait(&cond_barber_sleeping[barberId], &mMutex);
  }
  
  print(false, barberId, "starts a hair-cut service for customer[" + 
    int2string(customer_in_chair[barberId]) + "]");
  pthread_mutex_unlock(&mMutex);
}

// Processes a barber who finishes servicing a customer and requests for the
// next customer.
void Shop::byeCustomer(int barberId) {
  pthread_mutex_lock(&mMutex);

  // Hair Cut-Service is done so signal customer and wait for payment
  in_service[barberId] = false;
  print(false, barberId, "says he's done with a hair-cut service for customer[" 
    + int2string(customer_in_chair[barberId]) + "]");
  money_paid[barberId] = false;
  pthread_cond_signal(&cond_customer_served[barberId]);
  while (money_paid[barberId] == false) {
    pthread_cond_wait(&cond_barber_paid[barberId], &mMutex);
  }

  // Signal to customer to get next one
  customer_in_chair[barberId] = 0;
  in_chair_count--;
  print(false, barberId, "calls in another customer");
  pthread_cond_signal(&cond_customers_waiting);
  pthread_mutex_unlock(&mMutex);
}
