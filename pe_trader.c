#include "pe_trader.h"
//retest
int order_num = 0;
void handle_sig(int sig) {
    return;
}
void read_message(int read_fd, char *buff) {
    //read message and if number of bytes is -1 print error.
    int num_read = read(read_fd, buff, 256);
    if (num_read == -1) {
        perror("invalid!");
    }
}

int main(int argc, char ** argv) {
	//TODO implement your trader program to be fault-tolerant.

    if (argc < 2) {
        printf("Not enough arguments\n");
        return 1;
    }
    int id = atoi(argv[1]);

    //! register signal handler
    struct sigaction sig;
    memset(&sig, 0, sizeof(struct sigaction));
    sig.sa_flags = SA_SIGINFO | SA_RESTART;
    sig.sa_handler = handle_sig;

    //? block signals
    // sigemptyset(&sig.sa_mask);
    // sigaddset(&sig.sa_mask, SIGUSR1);
    if (sigaction(SIGUSR1, &sig, NULL) ==-1) {
        perror("sigaction failed");
    }


    //! connect to named pipes
    char exchange_pipe[MAX_FIFO_NAME];
    char auto_trader_pipe[MAX_FIFO_NAME];

    snprintf(exchange_pipe, MAX_FIFO_NAME, FIFO_EXCHANGE, id);
    snprintf(auto_trader_pipe, MAX_FIFO_NAME, FIFO_TRADER, id);
    
    int exchange_fd = open(exchange_pipe, O_RDONLY);
    int auto_trader_fd = open(auto_trader_pipe, O_WRONLY);
    if (exchange_fd == -1) {
        perror("Exchange read is invalid");
    }else if (auto_trader_fd == -1) {
        perror("AutoTrader read is invalid");
    }

    //! freeze program until autotrader process receive signal from exchange.
    pause();
    char buff[256];
    read(exchange_fd, buff, 256);
    memset(buff,0, 256);
    // event loop:
    while(1) {
        //TODO
        // wait for exchange update (MARKET message)
        // send order
        // wait for exchange confirmation (ACCEPTED message)

        //? wait until another message from market to come
        pause();
        /*
        sigset_t old_mask;
        sigemptyset(&sig.sa_mask);
        sigaddset(&sig.sa_mask, SIGUSR1);
        int result = sigprocmask(SIG_BLOCK, &sig.sa_mask, &old_mask);
        if (result == -1) {
            perror("error!!!");
        }
        */

        // receive message
        read_message(exchange_fd, buff);
        // remvoe ';' and parse message       
        strtok(buff,  ";");
        char *market = strtok(buff, " ");
        //validation
        if (strncmp(market, "MARKET", 6) != 0 ) {
            // perror("Invalid Message");
            memset(buff, 0, 256);
            continue;
        }else if (strncmp(market, "FILL", 4) == 0 ) {
            memset(buff, 0, 256);
            continue;
        }
        
        char *type = strtok(NULL, " ");
        char *product = strtok(NULL, " ");
        int amount_int = atoi(strtok(NULL, " "));
        int price_int = atoi(strtok(NULL, " "));
        if (strncmp(type, "SELL", 4) != 0) {
            memset(buff, 0, 256);
            continue;
        }else if (amount_int >= 1000) {
            break;
        }

        char order_str[128];
        int num_order_str =  snprintf(order_str, 128, "BUY %d %s %d %d;", order_num, product, amount_int, price_int);
        int num_write = write(auto_trader_fd, order_str, num_order_str);
        /*
        result = sigprocmask(SIG_SETMASK, &old_mask, NULL);
        if (result == -1) {
            perror("error!!");
        }
        sigprocmask(SIG_UNBLOCK, &old_mask, NULL);
        sigdelset(&sig.sa_mask, SIGUSR1);
        */        

        if (num_write == -1 || num_write == 0) {
            perror("Write to Exchange failed");
        }
        int rst = kill(getppid(), SIGUSR1);
        if (rst == -1) {
            perror("Signal did not send to exchange");
        }

        memset(buff,0,256);
        //? wait signal until trader receive accept order message from exchange
        sleep(2);
        read_message(exchange_fd, buff);
        /*
        result = sigprocmask(SIG_UNBLOCK, &sig.sa_mask, NULL);
        if (result == -1) {
            perror("error!!");
        }
        sigdelset(&sig.sa_mask, SIGUSR1);
        */
        memset(buff,0,256);

        order_num += 1;
    }
    

    close(exchange_fd);
    close(auto_trader_fd);

}