/**
 * comp2017 - assignment 3
 * Sangmyeong Lee
 * slee4736
 */

#include "pe_exchange.h"
int num_traders = 0;
int num_item = 0;
long long exchange_fee_collected = 0;
//! flag variables for signal handler
int IS_DISCONNECTED = 0;
int DISCONNECTED_PID = -1;
int IS_RECIEVED = 0; 

int RECIEVED_PID = -1;
int IS_EXIT = 0;
void handle_sig(int signum, siginfo_t *sig_info, void *vc) {
	if (signum == SIGUSR1) {
		// received message
		IS_RECIEVED = 1;
		RECIEVED_PID = sig_info -> si_pid;
	}else if (signum == SIGCHLD) {
		// child process is terminated
		IS_DISCONNECTED = 1;
		DISCONNECTED_PID = sig_info -> si_pid;
		// printf("disconnected %d\n", DISCONNECTED_PID);
	}else if (signum == SIGPIPE) {
		// invalid write to the pipe
		IS_DISCONNECTED = 1;
	}
}
void write_pipe(int fd, char *msg, Trader *trader) {
	if (fd == -1) {
		perror("FD is invalid\n");
		return;
	} else {
		if (trader -> is_online) {
			// DISCONNECTED_PID = trader -> pid;
			write(fd, msg, strlen(msg));
		}
	}
}
Item *create_item(char* name) {
	strtok(name, "\n");
    Item* new_item = (Item *)calloc(1, sizeof(Item));
    if (new_item == NULL) {
        printf("Failed to allocate new item\n");
        exit(1);
    }
	new_item -> name = calloc(MAX_ITEM_LEN + 1, sizeof(char));
	if (new_item -> name == NULL) {
		printf("Failed to allocate item name\n");
		exit(1);
	}
    strncpy(new_item->name, name, MAX_ITEM_LEN);
	new_item -> num_buy_ord = 0;
	new_item -> num_sell_ord = 0;
	new_item -> name[MAX_ITEM_LEN] = '\0';
    new_item -> prev = NULL;
    new_item -> next = NULL;
    new_item -> sell_orders = NULL;
    new_item -> buy_orders = NULL;
    return new_item;
}
Item *read_product(Item *head, char *path) {
	FILE *fptr;
	fptr = fopen(path, "r");
	if (fptr == NULL) {
		printf("Failed to Open Product Text");
		exit(1);
	}
	char buff[MAX_ITEM_LEN+1];
	if (fgets(buff, MAX_ITEM_LEN, fptr) != NULL) {
		num_item = atoi(buff);
		memset(buff, 0, MAX_ITEM_LEN);
	}
	if (num_item == 0) {
		printf("Invalid Number of Items");
		exit(1);
	}
	Item *ptr;
	for(int i = 0; i < num_item; i++) {
		if (fgets(buff, MAX_ITEM_LEN+1, fptr) != NULL) {
			Item *new_item = create_item(buff);
			if (head == NULL) {
				head = new_item; 
				ptr = head;
			}else {
				ptr -> next = new_item;
				new_item -> prev = ptr;
				ptr = new_item;
			}
			memset(buff, 0, MAX_ITEM_LEN);
		} else {
			printf("Invalid Item");
			exit(1);
		}
	}
	fclose(fptr);
	return head;
}
void print_items(Item *head) {
	Item *ptr = head;
	printf("%s Trading %d products: ", LOG_PREFIX, num_item);
	while(ptr -> next != NULL) {
		printf("%s ", ptr->name);
		ptr = ptr -> next;
	}
	if (ptr != NULL) {
		printf("%s\n", ptr->name);
	}
}
void free_orders(Order **orders, int num_order) {
	for(int i = 0; i < num_order; i++) {
		free(orders[i]);
	}
	free(orders);
}
void free_items(Item *head) {
	Item *ptr = head;
	while(ptr -> next != NULL) {
		Item *tmp;
		tmp = ptr->next;
		//TODO free sell_orders and buy_orders
		free_orders(ptr -> sell_orders, ptr -> num_sell_ord);
		free_orders(ptr -> buy_orders, ptr -> num_buy_ord);
		free(ptr->name);
		free(ptr);
		ptr = tmp;
	}
	if (ptr != NULL) {
		free_orders(ptr -> sell_orders, ptr -> num_sell_ord);
		free_orders(ptr -> buy_orders, ptr -> num_buy_ord);
		free(ptr->name);
		free(ptr);
	}
}
void free_inventories(Inventory **inventories) {
	for(int i = 0; i < num_item; i++) {
		free(inventories[i]->item_name);
		free(inventories[i]);
	}
	free(inventories);
}
void free_traders(Trader **traders, int num_traders) {
	for(int i = 0; i < num_traders; i++) {
		if (traders[i] != NULL) {
			free_inventories(traders[i]->inventories);
			free(traders[i]);
		}
	}
	free(traders);
}
void start_trader(Trader **traders, int id, char *path, Item *head, int num_item) {
	char exchange_pipe[MAX_FIFO_NAME];
	char trader_pipe[MAX_FIFO_NAME];
	snprintf(exchange_pipe, MAX_FIFO_NAME, FIFO_EXCHANGE, id);
	snprintf(trader_pipe, MAX_FIFO_NAME, FIFO_TRADER, id);

	unlink(exchange_pipe);
	if (mkfifo(exchange_pipe, 0666) == -1) {
		perror("Failed to open exchange pipe");
		traders[id] = NULL;
		return;
	}
	printf("%s Created FIFO %s\n", LOG_PREFIX, exchange_pipe);

	
	unlink(trader_pipe);
	if (mkfifo(trader_pipe, 0666) == -1) {
		perror("Failed to open trader pipe");
		traders[id] = NULL;
		return;
	}
	printf("%s Created FIFO %s\n", LOG_PREFIX, trader_pipe);
	//! starting trader
	//start trader on position id;
	printf("%s Starting trader %d (%s)\n", LOG_PREFIX, id, path);
	//? inventory generation for position output
	Inventory **inventories = (Inventory **)calloc(num_item, sizeof(Inventory *));
	Item *csr = head;
	int i = 0;
	while(csr -> next != NULL) {
		Inventory *new_inven = (Inventory *)calloc(1, sizeof(Inventory));
		new_inven->item_name = calloc(MAX_ITEM_LEN + 1, sizeof(char));
		if (new_inven->item_name == NULL) {
			perror("Failed to allocate inventory's item name\n");
			traders[id] = NULL;
			return;
		}
		strncpy(new_inven->item_name, csr -> name, MAX_ITEM_LEN);
		new_inven->item_name[MAX_ITEM_LEN] = '\0';
		new_inven->amount = 0;
		new_inven->balance = 0;
		inventories[i] = new_inven;
		i++;
		csr = csr -> next;
	}
	if (csr != NULL) {
		Inventory *new_inven = (Inventory *)calloc(1, sizeof(Inventory));
		new_inven->item_name = calloc(MAX_ITEM_LEN + 1, sizeof(char));
		if (new_inven->item_name == NULL) {
			perror("Failed to allocate inventory's item name\n");
			traders[id] = NULL;
			return;
		}
		strncpy(new_inven->item_name, csr -> name, MAX_ITEM_LEN);
		new_inven->item_name[MAX_ITEM_LEN] = '\0';
		new_inven->amount = 0;
		new_inven->balance = 0;
		inventories[i] = new_inven;
	}
	Trader *trader = (Trader *)calloc(1, sizeof(Trader));
	trader -> inventories = inventories;
	trader -> id = id;

	trader -> pid = fork();
	if (trader -> pid == -1) {
		perror("Fork failed\n");
		exit(1);
	} else if (trader -> pid == 0) {
		//child process
		//execl the path and send signal
		//max id is 999999
		char trader_id_arg[6];
		snprintf(trader_id_arg, 6, "%d", id);
		if (execl(path, path, trader_id_arg, NULL) == -1) {
			perror("Failed to execl the trader\n");
			exit(1);
		}
	} else {
		//parent process
		struct timespec time1;
		struct timespec time2;
		time1.tv_sec = 0;
		time1.tv_nsec = 1000000;
		nanosleep(&time1 , &time2);

		trader -> exchange_fd = open(exchange_pipe, O_WRONLY);
		trader -> trader_fd = open(trader_pipe, O_RDONLY);
		if (trader -> exchange_fd == -1) {
			printf("%s\n", exchange_pipe);
			perror("Exchange write is invalid\n");
			exit(1);
		}
		printf("%s Connected to %s\n", LOG_PREFIX, exchange_pipe);
		if (trader -> trader_fd == -1) {
			printf("%s\n", trader_pipe);
			perror("Trade read is invalid\n");
			exit(1);
		}
		printf("%s Connected to %s\n", LOG_PREFIX, trader_pipe);
		trader -> latest_order_id = -1;
		trader -> is_online = 1;
		traders[id] = trader;
		return;
	}
	return;
}
void disconnect_trader(Trader **traders) {
	int disconnected_traders = 0;
	for (int i = 0; i < num_traders; i++) {
		if (traders[i] == NULL || traders[i] -> is_online == 0) {
			disconnected_traders += 1;
		} else if (traders[i] -> pid == DISCONNECTED_PID && traders[i] -> is_online == 1) {
			//TODO close pipe
			traders[i] -> is_online = 0;
			printf("%s Trader %d disconnected\n", LOG_PREFIX, traders[i] -> id);
			char exchange_pipe[MAX_FIFO_NAME];
			char trader_pipe[MAX_FIFO_NAME];
			snprintf(exchange_pipe, MAX_FIFO_NAME, FIFO_EXCHANGE, traders[i]->id);
			snprintf(trader_pipe, MAX_FIFO_NAME, FIFO_TRADER, traders[i]->id);
			unlink(exchange_pipe);
			unlink(trader_pipe);
			disconnected_traders += 1;
			IS_DISCONNECTED = 0;
			DISCONNECTED_PID = -1;
		} else {
			// printf("Invalid disconnection is for trader id is %d and pid is %d\n", traders[i] -> id, traders[i] -> pid);
			// perror("Disconnection Error\n");
			continue;
		}
	}
	if (disconnected_traders == num_traders) {
		IS_EXIT = 1;
	}
	return;
}
void open_trader_inventories(Trader *found_trader, int num_item) {
	for(int i = 0; i < num_item; i++) {
		printf("found item is %s\n", found_trader->inventories[i]->item_name);
	}
}
Item *get_item(Item *head, char *item) {
	Item *csr = head;
	while(csr -> next != NULL) {
		if (strncmp(csr->name, item, MAX_ITEM_LEN) == 0) {
			return csr;
		}
		csr = csr -> next;
	}
	if (csr != NULL) {
		if (strncmp(csr->name, item, MAX_ITEM_LEN) == 0) {
			return csr;
		}
	}
	return NULL;
}
int is_valid_order(Item *head, Trader *found_trader, char *id, char *item, char *qty, char *price) {
	if (id == NULL || item == NULL || qty == NULL || price == NULL) {
		return -1;
	}
	//? check id range
	if(atoi(id) || (atoi(id) == 0 && strncmp("0", id, strlen(id)) == 0)) {
		int id_int = atoi(id);
		if (!(id_int >= 0 && id_int <= 999999)) {
			return -1;
		}
		if (found_trader -> latest_order_id + 1 != id_int) {
			return -1;
		}
	}else {
		return -1;
	}
	//? item
	if (get_item(head, item) == NULL) {
		return -1;
	}
	//? qty and price range
	if(atoi(qty)) {
		int qty_int = atoi(qty);
		if (!(qty_int >= 1 && qty_int <= 999999)) {
			return -1;
		}
	}else {
		return -1;
	}
	if(atoi(price)) {
		int price_int = atoi(price);
		if (!(price_int >= 1 && price_int <= 999999)) {
			return -1;
		}
	}else {
		return -1;
	}

	return 1;
}
int get_inventory_idx(Trader *trader, char *name) {
	for (int i = 0; i < num_item; i++) {
		if (strncmp((trader -> inventories)[i] -> item_name, name, MAX_ITEM_LEN) == 0) {
			return i;
		}
	}
	return -1;
}
void print_order(Item *csr) {
	printf("=======BUY ORDER for %s============\n", csr -> name);
	for (int i = 0; i < csr -> num_buy_ord; i++) {
		printf("ID: %d, COUNT: %d, PRICE: %d\n", (csr->buy_orders)[i]->ord_id, (csr->buy_orders)[i]->count, (csr->buy_orders)[i]->price);
	}
	printf("=======END BUY ORDER for %s============\n", csr -> name);
	printf("=======SELL ORDER for %s============\n", csr -> name);
	for (int i = 0; i < csr -> num_sell_ord; i++) {
		printf("ID: %d, COUNT: %d, PRICE: %d\n", (csr->sell_orders)[i]->ord_id, (csr->sell_orders)[i]->count, (csr->sell_orders)[i]->price);
	}
	printf("=======END SELL ORDER for %s============\n", csr -> name);
}
int comprator_buy_order(const void *ord1, const void *ord2) {
	Order *o1 = *(Order **)ord1;
	Order *o2 = *(Order **)ord2;
	if (o1 -> price > o2 -> price) {
		return -1;
	}else if (o1 -> price < o2 -> price) {
		return 1;
	}else {
		return 0;
	}
}
int comprator_sell_order(const void *ord1, const void *ord2) {
	Order *o1 = *(Order **)ord1;
	Order *o2 = *(Order **)ord2;
	if (o1 -> price < o2 -> price) {
		return -1;
	}else if (o1 -> price > o2 -> price) {
		return 1;
	}else {
		return 0;
	}
}
void resize_buy_orders(Item *item) {
	int new_length = 0;
	for (int i = 0; i < item -> num_buy_ord; i++) {
		if ((item -> buy_orders)[i] -> count != 0) {
			new_length += 1;
		}
	}
	Order **new_buy_orders = NULL;
	if (new_length != 0) {
		new_buy_orders = (Order **)malloc(new_length * sizeof(Order *));
		if (new_buy_orders == NULL) {
			return;
		}
	}
	int j = 0;
	for (int i = 0; i < item -> num_buy_ord; i++) {
		if ((item -> buy_orders)[i] -> count != 0) {
			new_buy_orders[j] = (item -> buy_orders)[i];
			j++;
		} else {
			free((item -> buy_orders)[i]);
		}
	}
	free(item -> buy_orders);
	item -> num_buy_ord = new_length;
	item -> buy_orders = new_buy_orders;
}
void resize_sell_orders(Item *item) {
	int new_length = 0;
	for (int i = 0; i < item -> num_sell_ord; i++) {
		if ((item -> sell_orders)[i] -> count != 0) {
			new_length += 1;
		}
	}
	Order **new_sell_orders = NULL;
	if (new_length != 0) {
		new_sell_orders = (Order **)malloc(new_length * sizeof(Order *));
		if (new_sell_orders == NULL) {
			return;
		}
	}
	int j = 0;
	for (int i = 0; i < item -> num_sell_ord; i++) {
		if ((item -> sell_orders)[i] -> count != 0) {
			new_sell_orders[j] = (item -> sell_orders)[i];
			j++;
		} else {
			free((item -> sell_orders)[i]);
		}
	}
	free(item -> sell_orders);
	item -> num_sell_ord = new_length;
	item -> sell_orders = new_sell_orders;
}
int get_level(Order **orders, int total_orders) {
	int level = 0;
	int prev_price = 0;
	for (int i = 0; i < total_orders; i++) {
		if (orders[i] -> price != prev_price) {
			prev_price = orders[i] -> price;
			level += 1;
		}
	}
	return level;
}
int get_total_num_ord(Order **orders, int len, int price) {
	int n = 0;
	for(int i = 0; i < len; i++) {
		if (orders[i] -> price == price) {
			n += 1;
		}
	}
	return n;
}
int get_total_cnt_ord(Order **orders, int len, int price) {
	int cnt = 0;
	for(int i = 0; i < len; i++) {
		if (orders[i] -> price == price) {
			cnt += orders[i] -> count;
		}
	}
	return cnt;
}
void print_order_by_level(Order **buy_orders, Order **sell_orders, int t_buy_ord, int t_sell_ord) {
	Order **orders = (Order **) malloc((t_buy_ord + t_sell_ord) * sizeof(Order *));
	for(int i = 0; i < t_buy_ord; i++) {
		orders[i] = buy_orders[i];
	}
	for(int i = 0; i < t_sell_ord; i++) {
		orders[i+t_buy_ord] = sell_orders[i];
	}
	qsort(orders, t_buy_ord + t_sell_ord, sizeof(Order *), comprator_buy_order);
	
	int i = 0;
	while (i < t_buy_ord + t_sell_ord) {
		Order *cur_ord = orders[i];
		printf("%s\t\t", LOG_PREFIX);
		if (cur_ord -> type == 0) {
			printf("SELL ");
		}else {
			printf("BUY ");

		}
		int total_num = get_total_num_ord(orders, t_buy_ord + t_sell_ord, cur_ord -> price);
		int total_cnt = get_total_cnt_ord(orders, t_buy_ord + t_sell_ord, cur_ord -> price);
		printf("%d @ $%d ", total_cnt, cur_ord -> price);
		if (total_num == 1) {
			printf("(1 order)\n");
		}else {
			printf("(%d orders)\n", total_num);
		}
		i += total_num;
	}
	// int is_first = 0;
	// int prev_price = 0;
	// int total_cnt = 0;
	// for (int i = 0; i < t_buy_ord + t_sell_ord; i++) {
	// 	if (orders[i] -> type == 0) {
	// 		printf("SELL ");
	// 	}else {
	// 		printf("BUY ");
	// 	}
	// 	printf("%d @ $%d\n", orders[i] -> count, orders[i] -> price);
	// }
	free(orders);
}
void print_order_book(Item *head) {
	printf("%s\t--ORDERBOOK--\n", LOG_PREFIX);
	Item *csr = head;
	while (csr -> next != NULL) {
		int buy_level  = get_level(csr -> buy_orders, csr -> num_buy_ord);
		int sell_level  = get_level(csr -> sell_orders, csr -> num_sell_ord);
		printf("%s\tProduct: %s; Buy levels: %d; Sell levels: %d\n", LOG_PREFIX, csr -> name, buy_level, sell_level);
		if (buy_level + sell_level > 0) {
			print_order_by_level(csr -> buy_orders, csr -> sell_orders, csr -> num_buy_ord, csr -> num_sell_ord);
		}
		csr = csr -> next;
	}
	if (csr != NULL) {
		int buy_level  = get_level(csr -> buy_orders, csr -> num_buy_ord);
		int sell_level  = get_level(csr -> sell_orders, csr -> num_sell_ord);
		printf("%s\tProduct: %s; Buy levels: %d; Sell levels: %d\n", LOG_PREFIX, csr -> name, buy_level, sell_level);
		if (buy_level + sell_level > 0) {
			print_order_by_level(csr -> buy_orders, csr -> sell_orders, csr -> num_buy_ord, csr -> num_sell_ord);
		}
	}
}
void print_positions(Trader **traders) {
	printf("%s\t--POSITIONS--\n", LOG_PREFIX);
	for(int i = 0; i < num_traders; i++) {
		if (traders[i] != NULL) {
			printf("%s\tTrader %d: ", LOG_PREFIX, i);
			Inventory **inventories = traders[i] -> inventories;
			for (int j = 0; j < num_item; j++) {
				if (j != num_item -1) {
					printf("%s %lld ($%lld), ", inventories[j] -> item_name, inventories[j] -> amount, inventories[j] -> balance);
				}else {
					printf("%s %lld ($%lld)", inventories[j] -> item_name, inventories[j] -> amount, inventories[j] -> balance);
				}
			}
			printf("\n");
		}
	}
}
void match_orders(Item *item, Trader **traders, Item *item_head, Order *new_order) {
	//TODO sort algorithm
	//! buy orders sort largest order
	qsort(item -> buy_orders, item -> num_buy_ord, sizeof(Order *), comprator_buy_order);
	//! sell orders sort smallest order
	qsort(item -> sell_orders, item -> num_sell_ord, sizeof(Order *), comprator_sell_order);

	if (new_order -> type == 0) {
		//sell order
		// buyer : loop to obtain.
		// seller: new order.
		for (int i = 0; i < item -> num_buy_ord; i++) {
			Order *old_buy_ord =  (item -> buy_orders)[i];
			if (old_buy_ord -> price >= new_order -> price) {
				//! order can be made
				long trade_count;
				char msg1[MAX_INPUT_LEN];
				char msg2[MAX_INPUT_LEN];
				if (old_buy_ord -> count < new_order -> count) {
					// multiple buy order may occur
					trade_count = (long) old_buy_ord -> count;
				}else {
					// finish match 
					trade_count = (long) new_order -> count;
				}
				snprintf(msg1, MAX_INPUT_LEN, "FILL %d %ld;", old_buy_ord -> ord_id, trade_count);
				snprintf(msg2, MAX_INPUT_LEN, "FILL %d %ld;", new_order -> ord_id, trade_count);
				Trader *cur_buy_trader = traders[old_buy_ord -> trader_id];
				Trader *cur_sell_trader = traders[new_order -> trader_id];
				exchange_fee_collected += (long long) round((long) trade_count * (long) (old_buy_ord -> price) * 0.01);
				if (traders[old_buy_ord -> trader_id] != NULL) {
					int inven_idx = get_inventory_idx(cur_buy_trader, item -> name);
					(cur_buy_trader -> inventories)[inven_idx] -> amount += (long long)trade_count;
					(cur_buy_trader -> inventories)[inven_idx] -> balance -= (long long)trade_count * (long long)old_buy_ord -> price;

					printf("%s Match: Order %d [T%d], New Order %d [T%d], value: $%lld, fee: $%lld.\n",LOG_PREFIX, 
						old_buy_ord-> ord_id, old_buy_ord -> trader_id, 
						new_order -> ord_id, new_order -> trader_id, 
						(long long) trade_count * (long long) (old_buy_ord -> price), (long long) round((long)trade_count * (long)(old_buy_ord -> price) * 0.01)
						);
				}
				//? find corresponding inventory for seller
				if (traders[new_order -> trader_id] != NULL) {
					int inven_idx = get_inventory_idx(cur_sell_trader, item -> name);
					(cur_sell_trader -> inventories)[inven_idx] -> amount -= (long long) trade_count;
					(cur_sell_trader -> inventories)[inven_idx] -> balance += (long long)trade_count * (long long)(old_buy_ord -> price) - (long long) round((long)trade_count * (long)(old_buy_ord -> price) * 0.01);
				}
				new_order -> count  -=  trade_count;
				old_buy_ord -> count -= trade_count;
				//write to pipe and send signal
				write_pipe(cur_buy_trader -> exchange_fd, msg1, cur_buy_trader);
				kill(cur_buy_trader -> pid, SIGUSR1);
				write_pipe(cur_sell_trader -> exchange_fd, msg2, cur_sell_trader);
				kill(cur_sell_trader -> pid, SIGUSR1);
				if (!(old_buy_ord -> count < new_order -> count)) {
					break;
				}
			} else {
				break;
			}
		}
	}else {
		//buy order
		// buyer : new order.
		// seller: loop to obtain.
		//check if multiple sell orders can be matched

		for (int i = 0; i < item -> num_sell_ord; i++) {
			Order *old_sell_ord =  (item -> sell_orders)[i];
			if (new_order -> price >= old_sell_ord -> price) {
				long trade_count;
				char msg1[MAX_INPUT_LEN];
				char msg2[MAX_INPUT_LEN];
				if (new_order -> count < old_sell_ord -> count) {
					// multiple buy order may occur
					trade_count = (long) new_order -> count;
				}else {
					// finish match 
					trade_count = (long) old_sell_ord -> count;
				}
				snprintf(msg1, MAX_INPUT_LEN, "FILL %d %ld;", new_order -> ord_id, trade_count);
				snprintf(msg2, MAX_INPUT_LEN, "FILL %d %ld;", old_sell_ord -> ord_id, trade_count);
				Trader *cur_buy_trader = traders[new_order -> trader_id];
				Trader *cur_sell_trader = traders[old_sell_ord -> trader_id];
				exchange_fee_collected += (long long) round((long)trade_count * (long)(old_sell_ord -> price) * 0.01);
				if (traders[new_order -> trader_id] != NULL) {
					int inven_idx = get_inventory_idx(cur_buy_trader, item -> name);
					(cur_buy_trader -> inventories)[inven_idx] -> amount += (long long)trade_count;
					(cur_buy_trader -> inventories)[inven_idx] -> balance -= (long long)trade_count * (long long)(old_sell_ord -> price) + (long long) round((long)trade_count * (long)(old_sell_ord -> price) * 0.01);
					printf("%s Match: Order %d [T%d], New Order %d [T%d], value: $%lld, fee: $%lld.\n",LOG_PREFIX, 
						old_sell_ord-> ord_id, old_sell_ord -> trader_id, 
						new_order -> ord_id, new_order -> trader_id, 
						(long long)((long) trade_count * (long)(old_sell_ord -> price)), (long long) round((long)trade_count * (long)(old_sell_ord -> price) * 0.01)
						);
				}
				//? find corresponding inventory for seller
				if (traders[old_sell_ord -> trader_id] != NULL) {
					int inven_idx = get_inventory_idx(cur_sell_trader, item -> name);
					(cur_sell_trader -> inventories)[inven_idx] -> amount -= (long long)trade_count;
					(cur_sell_trader -> inventories)[inven_idx] -> balance += (long long)trade_count * (long long)old_sell_ord -> price;
				}
				new_order -> count -= trade_count;
				old_sell_ord -> count  -= trade_count;
				//write to pipe and send signal
				write_pipe(cur_buy_trader -> exchange_fd, msg1, cur_buy_trader);
				kill(cur_buy_trader -> pid, SIGUSR1);
				write_pipe(cur_sell_trader -> exchange_fd, msg2, cur_sell_trader);
				kill(cur_sell_trader -> pid, SIGUSR1);


				if (!(new_order -> count < old_sell_ord -> count)) {
					break;
				}
			}else {
				break;
			}
		}
	}
	//update the order and resort.
	resize_buy_orders(item);
	resize_sell_orders(item);
	//TODO print orderbook and positions.
	print_order_book(item_head);
	print_positions(traders);
}
void place_order(Item *item, Trader **traders, Trader *found_trader, int is_buy, char *ord_id, char *amount, char *price, Item *item_head) {
	found_trader -> latest_order_id += 1;
	Order *new_order = (Order *) calloc(1, sizeof(Order));
	if (new_order == NULL) {
		//allocation failed!
		printf("New order allocation failed.\n");
		return;
	}
	new_order -> type = is_buy;
	new_order -> ord_id = found_trader -> latest_order_id;
	new_order -> trader_id = found_trader -> id;
	new_order -> count = atoi(amount);
	new_order -> price = atoi(price);

	if (is_buy) {
		// buy order
		if (item -> num_buy_ord == 0) {
			item -> num_buy_ord = 1;
			item -> buy_orders = (Order **)calloc(1, sizeof(Order *));
			if (item -> buy_orders == NULL) {
				printf("Buy orders allocation failed.\n");
				return;
			}
			(item -> buy_orders)[(item -> num_buy_ord)-1] = new_order;
		}else {
			item -> num_buy_ord += 1;
			item -> buy_orders = (Order **)realloc(item -> buy_orders, (item -> num_buy_ord) * sizeof(Order *));
			if (item -> buy_orders == NULL) {
				printf("Buy orders allocation failed.\n");
				return;
			}
			(item -> buy_orders)[(item -> num_buy_ord)-1] = new_order;
		}
	} else {
		// sell order
		if (item -> num_sell_ord == 0) {
			item -> num_sell_ord = 1;
			item -> sell_orders = (Order **)calloc(1, sizeof(Order *));
			if (item -> sell_orders == NULL) {
				printf("Sell orders allocation failed.\n");
				return;
			}
			(item -> sell_orders)[(item -> num_sell_ord)-1] = new_order;
		}else {
			item -> num_sell_ord += 1;
			item -> sell_orders = (Order **)realloc(item -> sell_orders, (item -> num_sell_ord) * sizeof(Order *));
			if (item -> sell_orders == NULL) {
				printf("Sell orders allocation failed.\n");
				return;
			}
			(item -> sell_orders)[(item -> num_sell_ord)-1] = new_order;
		}
	}
	//! SEND CONFIRMED MESSSAGE TO TRADERS
	char msg1[MAX_INPUT_LEN];
	snprintf(msg1, MAX_INPUT_LEN, "ACCEPTED %s;", ord_id);
	char msg2[MAX_INPUT_LEN];
	if (is_buy) {
		snprintf(msg2, MAX_INPUT_LEN, "MARKET BUY %s %s %s;", item->name, amount, price);
	}else {
		snprintf(msg2, MAX_INPUT_LEN, "MARKET SELL %s %s %s;", item->name, amount, price);
	}
	for (int i = 0; i < num_traders; i++) {
		if (traders[i] != NULL) {
			if (i == found_trader->id) {
				write_pipe(traders[i]->exchange_fd, msg1, traders[i]);
				kill(traders[i] -> pid, SIGUSR1);
			}else {
				write_pipe(traders[i]->exchange_fd, msg2, traders[i]);
				kill(traders[i] -> pid, SIGUSR1);
			}

		}
	}
	match_orders(item, traders, item_head, new_order);
}
void print_orders(Item *item_head) {
	Item *csr = item_head;
	while (csr -> next != NULL) {
		printf("=======BUY ORDER for %s============\n", csr -> name);
		for (int i = 0; i < csr -> num_buy_ord; i++) {
			printf("ID: %d, COUNT: %d, PRICE: %d\n", (csr->buy_orders)[i]->ord_id, (csr->buy_orders)[i]->count, (csr->buy_orders)[i]->price);
		}
		printf("=======END BUY ORDER for %s============\n", csr -> name);
		printf("=======SELL ORDER for %s============\n", csr -> name);
		for (int i = 0; i < csr -> num_sell_ord; i++) {
			printf("ID: %d, COUNT: %d, PRICE: %d\n", (csr->sell_orders)[i]->ord_id, (csr->sell_orders)[i]->count, (csr->sell_orders)[i]->price);
		}
		printf("=======END SELL ORDER for %s============\n", csr -> name);
		csr = csr -> next;
	}
	if (csr != NULL) {
		printf("=======BUY ORDER for %s============\n", csr -> name);
		for (int i = 0; i < csr -> num_buy_ord; i++) {
			printf("ID: %d, COUNT: %d, PRICE: %d\n", (csr->buy_orders)[i]->ord_id, (csr->buy_orders)[i]->count, (csr->buy_orders)[i]->price);
		}
		printf("=======END BUY ORDER for %s============\n", csr -> name);
		printf("=======SELL ORDER for %s============\n", csr -> name);
		for (int i = 0; i < csr -> num_sell_ord; i++) {
			printf("ID: %d, COUNT: %d, PRICE: %d\n", (csr->sell_orders)[i]->ord_id, (csr->sell_orders)[i]->count, (csr->sell_orders)[i]->price);
		}
		printf("=======END SELL ORDER for %s============\n", csr -> name);
	}
}

// Item *is_valid_cancel(Item *item_head, Trader **traders, Trader *found_trader, char *order_id) {
	// Item *csr = item_head;
	// return NULL;
// }

int is_valid_amend(Item *head, Trader *found_trader, char *id, char *qty, char *price) {
	if (id == NULL || qty == NULL || price == NULL) {
		return -1;
	}
	//? check id validity 
	if(atoi(id) || (atoi(id) == 0 && strncmp("0", id, strlen(id)) == 0)) {
		int id_int = atoi(id);
		if (!(id_int >= 0 && id_int <= 999999)) {
			return -1;
		}
		if (found_trader -> latest_order_id < id_int) {
			return -1;
		}
		Item *csr = head;
		int found = 0;
		while(csr -> next != NULL) {
			for (int i = 0; i < csr -> num_buy_ord; i++) {
				if ((csr -> buy_orders)[i] -> trader_id == found_trader -> id 
					&& (csr -> buy_orders)[i] -> ord_id == id_int) {
					found = 1;
					break;
				}
			}
			for (int i = 0; i < csr -> num_sell_ord; i++) {
				if ((csr -> sell_orders)[i] -> trader_id == found_trader -> id 
					&& (csr -> sell_orders)[i] -> ord_id == id_int) {
					found = 1;
					break;
				}
			}
			if (found) {
				break;
			}
			csr = csr -> next;
		}
		if (csr != NULL && found == 0) {
			for (int i = 0; i < csr -> num_buy_ord; i++) {
				if ((csr -> buy_orders)[i] -> trader_id == found_trader -> id 
					&& (csr -> buy_orders)[i] -> ord_id == id_int) {
					found = 1;
					break;
				}
			}
			for (int i = 0; i < csr -> num_sell_ord; i++) {
				if ((csr -> sell_orders)[i] -> trader_id == found_trader -> id 
					&& (csr -> sell_orders)[i] -> ord_id == id_int) {
					found = 1;
					break;
				}
			}
		}
		if (!found) {
			return -1;
		}
	}else {
		return -1;
	}
	//? qty and price range
	if(atoi(qty)) {
		int qty_int = atoi(qty);
		if (!(qty_int >= 1 && qty_int <= 999999)) {
			return -1;
		}
	}else {
		return -1;
	}
	if(atoi(price)) {
		int price_int = atoi(price);
		if (!(price_int >= 1 && price_int <= 999999)) {
			return -1;
		}
	}else {
		return -1;
	}
	return 1;
}
void place_amend(Item *item_head, Trader **traders, Trader *found_trader, char *ord_id, char *amount, char *price) {
	Item *csr = item_head;
	Item *found_item = NULL;
	Order *found_order = NULL;
	int item_idx;
	int ord_id_int = atoi(ord_id);
	int amount_int = atoi(amount);
	int price_int = atoi(price);
	//!get item and order
	while (csr -> next != NULL) {
		for (int i = 0; i < csr -> num_buy_ord; i++) {
			if ((csr -> buy_orders)[i] -> trader_id == found_trader -> id 
				&& (csr -> buy_orders)[i] -> ord_id == ord_id_int) {
				found_item = csr;
				found_order = (csr -> buy_orders)[i];
				item_idx = i;
				break;
			}
		}
		if (found_item != NULL && found_order != NULL) {
			break;
		}
		for (int i = 0; i < csr -> num_sell_ord; i++) {
			if ((csr -> sell_orders)[i] -> trader_id == found_trader -> id 
				&& (csr -> sell_orders)[i] -> ord_id == ord_id_int) {
				found_item = csr;
				found_order = (csr -> sell_orders)[i];
				item_idx = i;
				break;
			}
		}
		if (found_item != NULL && found_order != NULL) {
			break;
		}
		csr = csr -> next;
	}
	if (found_item == NULL && found_order == NULL && csr != NULL) { 
		for (int i = 0; i < csr -> num_buy_ord; i++) {
			if ((csr -> buy_orders)[i] -> trader_id == found_trader -> id
				&& (csr -> buy_orders)[i] -> ord_id == ord_id_int) {
				found_item = csr;
				found_order = (csr -> buy_orders)[i];
				item_idx = i;
				break;
			}
		}
		if (found_item == NULL && found_order == NULL) {
			for (int i = 0; i < csr -> num_sell_ord; i++) {
				if ((csr -> sell_orders)[i] -> trader_id == found_trader -> id 
					&& (csr -> sell_orders)[i] -> ord_id == ord_id_int) {
					found_item = csr;
					found_order = (csr -> sell_orders)[i];
					item_idx = i;
					break;
				}
			}
		}
	}
	found_order -> count = amount_int;
	found_order -> price = price_int;

	if (found_order -> type == 1) {
		// buy order
		Order *tmp = (found_item -> buy_orders)[(found_item -> num_buy_ord)-1];
		(found_item -> buy_orders)[item_idx] = tmp;
		(found_item -> buy_orders)[(found_item -> num_buy_ord)-1] = found_order;
	}else {
		// sell order
		Order *tmp = (found_item -> sell_orders)[(found_item -> num_sell_ord)-1];
		(found_item -> sell_orders)[item_idx] = tmp;
		(found_item -> sell_orders)[(found_item -> num_sell_ord)-1] = found_order;
	}
	//! SEND CONFIRMED MESSSAGE TO TRADERS
	char msg1[MAX_INPUT_LEN];
	snprintf(msg1, MAX_INPUT_LEN, "AMENDED %s;", ord_id);
	char msg2[MAX_INPUT_LEN];
	if (found_order -> type) {
		snprintf(msg2, MAX_INPUT_LEN, "MARKET BUY %s %d %d;", found_item -> name, found_order -> count, found_order -> price);
	}else {
		snprintf(msg2, MAX_INPUT_LEN, "MARKET SELL %s %d %d;", found_item -> name, found_order -> count, found_order -> price);
	}
	for (int i = 0; i < num_traders; i++) {
		if (traders[i] != NULL) {
			if (i == found_trader->id) {
				write_pipe(traders[i]->exchange_fd, msg1, traders[i]);
				kill(traders[i] -> pid, SIGUSR1);
			}else {
				write_pipe(traders[i]->exchange_fd, msg2, traders[i]);
				kill(traders[i] -> pid, SIGUSR1);
			}

		}
	}
	match_orders(found_item, traders, item_head, found_order);
}



int is_valid_cancel(Item *head, Trader *found_trader, char *id) {
	if (id == NULL) {
		return -1;
	}
	//? check id range
	if(atoi(id) || (atoi(id) == 0 && strncmp("0", id, strlen(id)) == 0)) {
		int id_int = atoi(id);
		if (!(id_int >= 0 && id_int <= 999999)) {
			return -1;
		}
		if (found_trader -> latest_order_id < id_int) {
			return -1;
		}
		Item *csr = head;
		int found = 0;
		while(csr -> next != NULL) {
			for (int i = 0; i < csr -> num_buy_ord; i++) {
				if ((csr -> buy_orders)[i] -> trader_id == found_trader -> id 
					&& (csr -> buy_orders)[i] -> ord_id == id_int) {
					found = 1;
					break;
				}
			}
			if (found) {
				return 1;
			}
			for (int i = 0; i < csr -> num_sell_ord; i++) {
				if ((csr -> sell_orders)[i] -> trader_id == found_trader -> id 
					&& (csr -> sell_orders)[i] -> ord_id == id_int) {
					found = 1;
					break;
				}
			}
			if (found) {
				return 1;
			}
			csr = csr -> next;
		}
		if (csr != NULL && found == 0) {
			for (int i = 0; i < csr -> num_buy_ord; i++) {
				if ((csr -> buy_orders)[i] -> trader_id == found_trader -> id 
					&& (csr -> buy_orders)[i] -> ord_id == id_int) {
					found = 1;
					break;
				}
			}
			if (found) {
				return 1;
			}
			for (int i = 0; i < csr -> num_sell_ord; i++) {
				if ((csr -> sell_orders)[i] -> trader_id == found_trader -> id 
					&& (csr -> sell_orders)[i] -> ord_id == id_int) {
					found = 1;
					break;
				}
			}
		}
		if (!found) {
			return -1;
		}
	}else {
		return -1;
	}
	return 1;
}
void place_cancel(Item *item_head, Trader **traders, Trader *found_trader, char *ord_id) {

	Item *csr = item_head;
	int ord_id_int = atoi(ord_id);
	Item *found_item = NULL;
	Order *found_order = NULL;
	//!get item and order
	while (csr -> next != NULL) {
		for (int i = 0; i < csr -> num_buy_ord; i++) {
			if ((csr -> buy_orders)[i] -> trader_id == found_trader -> id 
				&& (csr -> buy_orders)[i] -> ord_id == ord_id_int) {
				(csr -> buy_orders)[i] -> count = 0;
				(csr -> buy_orders)[i] -> price = 0;
				found_item = csr;
				found_order = (csr -> buy_orders)[i];
				// resize_buy_orders(csr);
				break;
			}
		}
		if (found_item != NULL && found_order != NULL) {
			break;
		}
		for (int i = 0; i < csr -> num_sell_ord; i++) {
			if ((csr -> sell_orders)[i] -> trader_id == found_trader -> id 
				&& (csr -> sell_orders)[i] -> ord_id == ord_id_int) {
				(csr -> sell_orders)[i] -> count = 0;
				(csr -> sell_orders)[i] -> price = 0;
				found_item = csr;
				found_order = (csr -> sell_orders)[i];
				// resize_sell_orders(csr);
				break;
			}
		}
		if (found_item != NULL && found_order != NULL) {
			break;
		}
		csr = csr -> next;
	}
	if (found_item == NULL && found_order == NULL && csr != NULL) { 
		for (int i = 0; i < csr -> num_buy_ord; i++) {
			if ((csr -> buy_orders)[i] -> trader_id == found_trader -> id
				&& (csr -> buy_orders)[i] -> ord_id == ord_id_int) {
				(csr -> buy_orders)[i] -> count = 0;
				(csr -> buy_orders)[i] -> price = 0;
				found_item = csr;
				found_order = (csr -> buy_orders)[i];
				// resize_buy_orders(csr);
				break;
			}
		}
		if (found_item == NULL && found_order == NULL) {
			for (int i = 0; i < csr -> num_sell_ord; i++) {
				if ((csr -> sell_orders)[i] -> trader_id == found_trader -> id 
					&& (csr -> sell_orders)[i] -> ord_id == ord_id_int) {
					(csr -> sell_orders)[i] -> count = 0;
					(csr -> sell_orders)[i] -> price = 0;
					found_item = csr;
					found_order = (csr -> sell_orders)[i];
					// resize_sell_orders(csr);
					break;
				}
			}
		}
	}
	char msg1[MAX_INPUT_LEN];
	snprintf(msg1, MAX_INPUT_LEN, "CANCELLED %s;", ord_id);
	char msg2[MAX_INPUT_LEN];
	if (found_order -> type) {
		snprintf(msg2, MAX_INPUT_LEN, "MARKET BUY %s %d %d;", found_item -> name, 0, 0);
		resize_buy_orders(found_item);
	}else {
		snprintf(msg2, MAX_INPUT_LEN, "MARKET SELL %s %d %d;", found_item -> name, 0, 0);
		resize_sell_orders(found_item);
	}
	for (int i = 0; i < num_traders; i++) {
		if (traders[i] != NULL) {
			if (i == found_trader->id) {
				write_pipe(traders[i]->exchange_fd, msg1, traders[i]);
				kill(traders[i] -> pid, SIGUSR1);
			}else {
				write_pipe(traders[i]->exchange_fd, msg2, traders[i]);
				kill(traders[i] -> pid, SIGUSR1);
			}

		}
	}
	// qsort(found_item -> buy_orders, found_item -> num_buy_ord, sizeof(Order *), comprator_buy_order);
	// qsort(found_item -> sell_orders, found_item -> num_sell_ord, sizeof(Order *), comprator_sell_order);
	print_order_book(item_head);
	print_positions(traders);
}
int parse_input(Item *item_head, Trader **traders, Trader *found_trader, char *input, int num_traders) {
	printf("%s [T%d] Parsing command: <%s>\n", LOG_PREFIX, found_trader -> id, input);
    //validation
    char *type = strtok(input, " ");
	if (strncmp(type, "BUY", 3) == 0 || strncmp(type, "SELL", 4) == 0) {
		char *order_id = strtok(NULL, " ");
		char *item = strtok(NULL, " ");
		char *amount = strtok(NULL, " ");
		char *price = strtok(NULL, " ");
		int result = is_valid_order(item_head, found_trader, order_id, item, amount, price);
		if (result == -1) {
			return -1;
		}
		//! ensure there is no more parameters
		char *has_to_be_null = strtok(NULL, " ");
		if (has_to_be_null != NULL) {
			return -1;
		}
		int is_buy = 0;
		if (strncmp(type, "BUY", 3) == 0) {
			is_buy = 1;
		}
		place_order(get_item(item_head, item), traders, found_trader, is_buy, order_id, amount, price, item_head);
	}else if(strncmp(type, "AMEND", 5) == 0) {
		char *order_id = strtok(NULL, " ");
		char *amount = strtok(NULL, " ");
		char *price = strtok(NULL, " ");
		int result = is_valid_amend(item_head, found_trader, order_id, amount, price);
		if (result == -1) {
			return -1;
		}
		//! ensure there is no more parameters
		char *has_to_be_null = strtok(NULL, " ");
		if (has_to_be_null != NULL) {
			return -1;
		}
		place_amend(item_head, traders, found_trader, order_id, amount, price);

	}else if(strncmp(type, "CANCEL", 6) == 0) {
		char *order_id = strtok(NULL, " ");
		int result = is_valid_cancel(item_head, found_trader, order_id);
		if (result == -1) {
			return -1;
		}
		//! ensure there is no more parameters
		char *has_to_be_null = strtok(NULL, " ");
		if (has_to_be_null != NULL) {
			return -1;
		}
		place_cancel(item_head, traders, found_trader, order_id);
	}else {
		return -1;
	}
	return 1;
}
int main(int argc, char **argv) {
	if (argc < 3) {
		printf("Invalid amount of arguments");
		return -1;
	}
	printf("%s Starting\n", LOG_PREFIX);
	//!read products
	Item *item_head = NULL;
	item_head = read_product(item_head, argv[1]);
	print_items(item_head);
	//!build signal handlers
	struct sigaction sig;
    memset(&sig, 0, sizeof(struct sigaction));
    sig.sa_flags = SA_SIGINFO | SA_RESTART;
    sig.sa_handler = (void *)handle_sig;
	//attach handlers
	if (sigaction(SIGUSR1, &sig, NULL) == -1) {
        perror("sigaction failed");
    }
	if (sigaction(SIGCHLD, &sig, NULL) == -1) {
        perror("sigaction failed");
    }
	if (sigaction(SIGPIPE, &sig, NULL) == -1) {
        perror("sigaction failed");
    }
	num_traders = argc - 2; 
	Trader **traders = (Trader **)calloc(num_traders, sizeof(Trader *));
	//start traders
	for (int i = 0; i < num_traders; i++) {
		start_trader(traders, i, argv[i+2], item_head, num_item);
	}
	//! market open
	for(int i = 0; i < num_traders; i++) {
		if(traders[i] != NULL) {
			write_pipe(traders[i]->exchange_fd, "MARKET OPEN;", traders[i]);
			kill(traders[i] -> pid, SIGUSR1);
		}
	}
	// for (int i = 0 ; i< num_traders; i++) {
	// 	printf("pid is %d\n", traders[i] -> pid);
	// }
	while (1) {
		if (IS_RECIEVED == 0 && IS_DISCONNECTED == 0) {
			pause();
		}
		if (IS_DISCONNECTED) {
			disconnect_trader(traders);
		}
		if (IS_EXIT) {
			printf("%s Trading completed\n", LOG_PREFIX);
			printf("%s Exchange fees collected: $%lld\n", LOG_PREFIX, exchange_fee_collected);
			break;
		}
		//read sent message
		if (IS_RECIEVED && RECIEVED_PID > -1) {
			IS_RECIEVED = 0;
			Trader *found_trader = NULL;
			for (int i = 0; i < num_traders; i++) {
				if (traders[i] -> pid == RECIEVED_PID) {
					found_trader = traders[i];
					break;
				}
			}
			RECIEVED_PID = -1;
			char input[MAX_INPUT_LEN];
			memset(input, 0, MAX_INPUT_LEN);
			if (found_trader != NULL) {
				read(found_trader -> trader_fd, input, MAX_INPUT_LEN);
				//! parse input;
				strtok(input, ";");
				// open_trader_inventories(found_trader, num_item);
				int rst = parse_input(item_head, traders, found_trader, input, num_traders);
				if (rst == -1) {
					//! send invalid and send signal
					write_pipe(found_trader -> exchange_fd, "INVALID;", found_trader);
					kill(found_trader -> pid, SIGUSR1);
				}
				// print_orders(item_head);
			}else {
				//!
				printf("invalid read from trader\n");
			}
		}else {
			pause();
		}
	}

	free_items(item_head);
	free_traders(traders, num_traders);
	return 0;
}