# ProgExchange(PEX) Virtual Auto-Trading Program
![C](https://img.shields.io/badge/c-%2300599C.svg?style=flat-square&logo=c&logoColor=white)
![Linux](https://img.shields.io/badge/Linux-FCC624?style=flat-square&logo=linux&logoColor=black)
<a href="https://github.com/fan2goa1/best-of-usyd" title="best of USYD"><img alt="best of USYD" src="https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/fan2goa1/best-of-usyd/main/assets/badge/v1.json"></a>

## Descripton
Introducing ProgExchange (PEX), a cutting-edge platform for computer programmers to buy and sell
high-demand components in a virtual marketplace. The need for in-person trading has been replaced
by a state-of-the-art digital marketplace, allowing for greater accessibility and faster transactions,
while providing a safe and convenient way for seasoned programmers to connect and make transactions.
As part of this assignment, you will be tasked with developing two key components of ProgExchange:
the exchange itself, which will handle all incoming orders, and an auto-trading program that executes
trades based on predefined conditions. With your expertise in systems programming, you will play
a crucial role in bringing this innovative platform to life and helping to drive the future of computer
component trading.
## Getting Started
### 1. Describe how your exchange works.
The exchange starts by reading product.txt file. Each item will be initialised as a Struct Item object and formed as a linked list. Followed by creation of Item linked list, the exchange will start traders' binary files by creating and connecting FIFO pipe. 
Every time, trader process send the signal to read pipe, exchange will handle this by flag manipulation in the signal handler in order to read in evemt loop. Where if any trader did not send signal, event loop will call pause to save CPU consumption.
Each order will be differentiated by SELL or BUY and stored into the struct Item. They also store all information of whose trader belong to so it is easier to match.
Matching order will be followed as spec that new buy order will be matched by the lowest price of sell order. Every time order parsed, order book and positions of traders will be printed.

### 2. Describe your design decisions for the trader and how it's fault-tolerant.
To ensure auto trader fault tolerant, I use pause() to wait process receive signal from the exchange. Before event loop executed, the auto trader paused to wait for receiving MARKET OPEN message from exchange. Followed by receiving open message from exchange, event loop will puase until another signal received. 
Another method is by preventing signal loss. To achieve this issue, I set SA_RESTART in sa_flag so instead of returning error while it will wait until current operation finishes. 



## Reference
This implementation was assignment 3 for COMP2017. 
