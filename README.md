# ProgExchange(PEX) Virtual Auto-Trading Program
![C](https://img.shields.io/badge/c-%2300599C.svg?style=flat-square&logo=c&logoColor=white)
![Linux](https://img.shields.io/badge/Linux-FCC624?style=flat-square&logo=linux&logoColor=black)
<a href="https://github.com/fan2goa1/best-of-usyd" title="best of USYD"><img alt="best of USYD" src="https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/fan2goa1/best-of-usyd/main/assets/badge/v1.json"></a>


## Description
Introducing ProgExchange (PEX), a cutting-edge platform for computer programmers to buy and sell high-demand components in a virtual marketplace. This platform eliminates the need for in-person trading by offering a state-of-the-art digital marketplace that enhances accessibility and accelerates transactions. It provides a safe and convenient environment for seasoned programmers to connect and conduct their trades.

Two key components of ProgExchange: the exchange, which handles all incoming orders, and an auto-trading program that executes trades based on predefined conditions. Expertising in systems programming will be essential in bringing this innovative platform to life and shaping the future of computer component trading.

## Getting Started

### 1. How the Exchange Works
The exchange starts by reading the `product.txt` file. Each item is initialized as a `Struct Item` object and organized into a linked list. After creating the Item linked list, the exchange launches the traders' binary files by creating and connecting FIFO pipes.

Whenever a trader process sends a signal to the read pipe, the exchange handles it by manipulating flags in the signal handler, allowing it to respond within an event loop. If no trader sends a signal, the event loop invokes `pause()` to reduce CPU consumption.

Each order is classified as either a SELL or BUY and stored within the `Struct Item`, along with details about the trader, making it easier to match orders. Orders are matched according to the specification, where a new buy order is matched with the lowest-priced sell order. After each order is processed, the order book and traders' positions are updated and printed.

### 2. Design Decisions for the Trader and Fault Tolerance
To ensure the auto-trader is fault-tolerant, `pause()` is used to wait for signals from the exchange. Before executing the event loop, the auto-trader pauses until it receives a MARKET OPEN message from the exchange. Following this, the event loop pauses again until another signal is received.

Another method used to prevent signal loss is setting the `SA_RESTART` flag in `sa_flags`. This ensures that instead of returning an error, the process waits until the current operation is completed.

## Reference
This project is the 2023 COMP2017 Assignment 3. Feel free to fork this repository and modify any code that may be missing.
