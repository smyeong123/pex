#ifndef PE_EXCHANGE_H
#define PE_EXCHANGE_H

#include "pe_common.h"

#define LOG_PREFIX "[PEX]"
#define MAX_ITEM_LEN 17
#define MAX_INPUT_LEN 64
typedef struct Order{
    //! 0 = sell, 1 = buy;
    int type;
    int ord_id;
    int trader_id;
    int count;
    int price;
}Order;

typedef struct Item{
    char *name;
    int num_buy_ord;
    int num_sell_ord;
    struct Item *prev;
    struct Item *next;
    Order **buy_orders;
    Order **sell_orders;
}Item;

typedef struct Inventory{
    char *item_name;
    long long amount;
    long long balance;
}Inventory;

typedef struct Trader{
    //! 0 = dead, 1 = online;
    int is_online;
    int latest_order_id;
    int id;
    int exchange_fd;    
    int trader_fd;
    pid_t pid;
    struct Inventory **inventories;
}Trader;
#endif
