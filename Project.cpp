#include <iostream>
#include <cstring>
#include <fstream>
#include <ctime>

#pragma warning(disable: 4996)

const long long SYSTEM_WALLET_ID = 4294967295;
const size_t INITIAL_CAPACITY = 2;
const int EXCHANGE_RATE = 375;
const int RICHEST_USERS_COUNT = 10;
const size_t MAX_INPUT_LENGTH = 1024;

const char WALLETS_FILENAME[] = "wallets.dat";
const char EXECUTED_ORDERS_FILENAME[] = "executed_orders.dat";
const char TRANSACTIONS_FILENAME[] = "transactions.dat";
const char ORDERS_FILENAME[] = "orders.dat";

struct Wallet {
    char owner[256];
    unsigned id;
    double fiatMoney;
};

struct WalletContainer {
    Wallet* items;
    size_t* executedOrders;
    size_t count, capacity;
};

struct Transaction {
    long long time;
    unsigned senderId;
    unsigned receiverId;
    double grnCoins;
};

struct TransactionContainer {
    Transaction* items;
    size_t count, capacity;
};

struct Order {
    enum Type { SELL, BUY } type;
    unsigned walletId;
    double grnCoins;
};

struct OrdersContainer {
    Order* items;
    bool* executed;
    size_t count, capacity;
};

struct System {
    WalletContainer wallets;
    TransactionContainer transactions;
    OrdersContainer orders;
};

void resizeWalletContainer(System& system) {
    Wallet* newWallets = new (std::nothrow) Wallet[system.wallets.capacity *= 2];
    size_t* newExecutedOrders = new (std::nothrow) size_t[system.wallets.capacity];
    for (size_t i = 0; i < system.wallets.count; i++) {
        newWallets[i] = system.wallets.items[i];
        newExecutedOrders[i] = system.wallets.executedOrders[i];
    }
    delete[] system.wallets.items;
    delete[] system.wallets.executedOrders;
    system.wallets.items = newWallets;
    system.wallets.executedOrders = newExecutedOrders;
}

void resizeTransactionContainer(System& system) {
    Transaction* newTransactions = new (std::nothrow) Transaction[system.transactions.capacity *= 2];
    for (size_t i = 0; i < system.transactions.count; i++) {
        newTransactions[i] = system.transactions.items[i];
    }
    delete[] system.transactions.items;
    system.transactions.items = newTransactions;
}

void resizeOrderContainer(System& system) {
    Order* newOrders = new (std::nothrow) Order[system.orders.capacity *= 2];
    bool* newExecuted = new (std::nothrow) bool[system.orders.capacity];
    for (size_t i = 0; i < system.orders.count; i++) {
        newOrders[i] = system.orders.items[i];
        newExecuted[i] = system.orders.executed[i];
    }
    delete[] system.orders.items;
    delete[] system.orders.executed;
    system.orders.items = newOrders;
    system.orders.executed = newExecuted;
}

unsigned generateId() {
    unsigned number = 0;
    for (size_t i = 0; i < 32; i++) {
        int randomBit = rand() % 2;
        number = number | (randomBit << i);
    }
    return number;
}

long long getTime() {
    long long seconds = time(NULL);
    return seconds;
}

Wallet* findWallet(const System& system, const unsigned walletId) {
    for (size_t i = 0; i < system.wallets.count; i++) {
        if (system.wallets.items[i].id == walletId) {
            return &system.wallets.items[i];
        }
    }
    return nullptr;
}

double getCoins(const System& system, const unsigned walletId) {
    double balance = 0;
    for (size_t i = 0; i < system.transactions.count; i++) {
        if (system.transactions.items[i].senderId == walletId) {
            balance -= system.transactions.items[i].grnCoins;
        }
        else if (system.transactions.items[i].receiverId == walletId) {
            balance += system.transactions.items[i].grnCoins;
        }
    }
    return balance;
}

bool transfer(System& system, const unsigned senderId, const unsigned receiverId, const double grnCoins) {
    if ((findWallet(system, senderId) == nullptr && senderId != SYSTEM_WALLET_ID) ||
        findWallet(system, receiverId) == nullptr ||
        (getCoins(system, senderId) < grnCoins && senderId != SYSTEM_WALLET_ID)) {
        return false;
    }

    Transaction transaction;
    transaction.senderId = senderId;
    transaction.receiverId = receiverId;
    transaction.grnCoins = grnCoins;
    transaction.time = getTime();

    if (system.transactions.count == system.transactions.capacity) {
        resizeTransactionContainer(system);
    }
    system.transactions.items[system.transactions.count++] = transaction;

    return true;
}

long long addWallet(System& system, const double fiatMoney, const char name[]) {
    if (strlen(name) <= 255) {
        Wallet wallet;
        strcpy(wallet.owner, name);
        do {
            wallet.id = generateId();
        } 
        while (findWallet(system, wallet.id));
        wallet.fiatMoney = fiatMoney;

        if (system.wallets.count == system.wallets.capacity) {
            resizeWalletContainer(system);
        }
        system.wallets.items[system.wallets.count++] = wallet;

        if (transfer(system, SYSTEM_WALLET_ID, wallet.id, wallet.fiatMoney / EXCHANGE_RATE)) {
            return wallet.id;
        }
    }
    return -1;
}

void walletInfo(const System& system, const unsigned walletId) {
    Wallet* wallet = findWallet(system, walletId);
    if (wallet != nullptr) {
        std::cout << "Owner: " << wallet->owner << std::endl;
        std::cout << "Fiat money: " << wallet->fiatMoney << std::endl;
        std::cout << "GRN coins: " << getCoins(system, walletId) << std::endl;
    }
    else {
        std::cout << "There is no wallet with ID: " << walletId << std::endl;
    }
}

size_t executedOrders(const System& system, const unsigned walletId) {
    for (size_t i = 0; i < system.wallets.count; i++) {
        if (system.wallets.items[i].id == walletId) {
            return system.wallets.executedOrders[i];
        }
    }
    return 0;
}

long long getTimeFirstOrder(const System& system, const unsigned walletId) {
    for (size_t i = 0; i < system.transactions.count; i++) {
        if (system.transactions.items[i].receiverId == walletId ||
            system.transactions.items[i].senderId == walletId) {
            return system.transactions.items[i].time;
        }
    }
    return -1;
}

long long getTimeLastOrder(const System& system, const unsigned walletId) {
    for (size_t i = system.transactions.count - 1; i >= 0; i--) {
        if (system.transactions.items[i].receiverId == walletId ||
            system.transactions.items[i].senderId == walletId) {
            return system.transactions.items[i].time;
        }
    }
    return -1;
}

void richUserInfo(const System& system, const unsigned walletId) {
    Wallet* wallet = findWallet(system, walletId);
    if (wallet != nullptr) {
        std::cout << "Owner: " << wallet->owner << std::endl;
        std::cout << "Wallet ID: " << wallet->id << std::endl;
        std::cout << "GRN coins: " << getCoins(system, walletId) << std::endl;
        size_t executedOrdersCount = executedOrders(system, walletId);
        std::cout << "Executed orders: " << executedOrdersCount << std::endl;
        if (executedOrdersCount!=0) {
            std::cout << "First order executed at: " << getTimeFirstOrder(system, walletId) << std::endl;
            std::cout << "Last order executed at: " << getTimeLastOrder(system, walletId) << std::endl;
        }
    }
    else {
        std::cout << "There is no wallet with ID: " << walletId << std::endl;
    }
}

void swap(Wallet* wallet1, Wallet* wallet2) {
    Wallet swap = *wallet1;
    *wallet1 = *wallet2;
    *wallet2 = swap;
}

void attractInvestors(const System& system) {
    for (size_t i = 0; i < RICHEST_USERS_COUNT; i++) {
        unsigned maxCoinsWalletId = system.wallets.items[i].id;
        for (size_t j = i + 1; j < system.wallets.count; j++) {
            if (getCoins(system, maxCoinsWalletId) < getCoins(system, system.wallets.items[j].id)) {
                maxCoinsWalletId = system.wallets.items[j].id;
            }
        }
        swap(findWallet(system, maxCoinsWalletId),
            findWallet(system, system.wallets.items[i].id));
        richUserInfo(system, maxCoinsWalletId);
    }
}

void generateTextFile(const System& system) {

}

void executeOrders(System& system) {
    for (size_t i = 0; i < system.orders.count; i++) {
        if (system.orders.items[i].type == Order::Type::BUY && !(system.orders.executed[i])) {
            for (size_t j = i + 1; j < system.orders.count; j++) {
                if (system.orders.items[j].type == Order::Type::SELL && !(system.orders.executed[j])) {
                    double buyCoins = system.orders.items[i].grnCoins;
                    double sellCoins = system.orders.items[j].grnCoins;
                    if (buyCoins >= sellCoins) {
                        transfer(system, system.orders.items[j].walletId, system.orders.items[i].walletId,
                            sellCoins);
                        findWallet(system, system.orders.items[i].walletId)->fiatMoney -= sellCoins * EXCHANGE_RATE;
                        findWallet(system, system.orders.items[j].walletId)->fiatMoney += sellCoins * EXCHANGE_RATE;
                        system.orders.executed[j] = true;
                        
                    }
                    else {
                        transfer(system, system.orders.items[j].walletId, system.orders.items[i].walletId,
                            buyCoins);
                        findWallet(system, system.orders.items[i].walletId)->fiatMoney -= buyCoins * EXCHANGE_RATE;
                        findWallet(system, system.orders.items[j].walletId)->fiatMoney += buyCoins * EXCHANGE_RATE;
                        system.orders.executed[i] = true;
                    }
                }
            }
        }
        else if (system.orders.items[i].type == Order::Type::SELL && !(system.orders.executed[i])) {
            for (size_t j = i + 1; j < system.orders.count; j++) {
                if (system.orders.items[j].type == Order::Type::BUY && !(system.orders.executed[j])) {
                    double sellCoins = system.orders.items[i].grnCoins;
                    double buyCoins = system.orders.items[j].grnCoins;
                    if (buyCoins >= sellCoins) {
                        transfer(system, system.orders.items[i].walletId, system.orders.items[j].walletId,
                            sellCoins);
                        findWallet(system, system.orders.items[i].walletId)->fiatMoney += sellCoins * EXCHANGE_RATE;
                        findWallet(system, system.orders.items[j].walletId)->fiatMoney -= sellCoins * EXCHANGE_RATE;
                        system.orders.executed[i] = true;

                    }
                    else {
                        transfer(system, system.orders.items[i].walletId, system.orders.items[j].walletId,
                            buyCoins);
                        findWallet(system, system.orders.items[i].walletId)->fiatMoney += buyCoins * EXCHANGE_RATE;
                        findWallet(system, system.orders.items[j].walletId)->fiatMoney -= buyCoins * EXCHANGE_RATE;
                        system.orders.executed[j] = true;
                    }
                }
            }
        }
    }
}

double buyerUsableMoney(const System& system, const unsigned walletId) {
    double usableMoney = findWallet(system, walletId)->fiatMoney;
    for (size_t i = 0; i < system.orders.count; i++) {
        if (system.orders.items[i].walletId = walletId &&
            system.orders.items[i].type == Order::Type::BUY &&
            !(system.orders.executed[i])) {
            usableMoney -= system.orders.items[i].grnCoins * EXCHANGE_RATE;
        }
    }
    return usableMoney;
}

double sellerUsableCoins(const System& system, const unsigned walletId) {
    double usableCoins = getCoins(system, walletId);
    for (size_t i = 0; i < system.orders.count; i++) {
        if (system.orders.items[i].walletId = walletId &&
            system.orders.items[i].type == Order::Type::SELL &&
            !(system.orders.executed[i])) {
            usableCoins -= system.orders.items[i].grnCoins;
        }
    }
    return usableCoins;
}

bool addOrder(System& system, const unsigned walletId, const Order::Type type, const double grnCoins) {
    if (findWallet(system, walletId) == nullptr) {
        return false;
    }

    if (type == Order::Type::BUY) {
        if (buyerUsableMoney(system,walletId) < grnCoins * EXCHANGE_RATE) {
            return false;
        }

        Order order;
        order.type = Order::Type::BUY;
        order.walletId = walletId;
        order.grnCoins = grnCoins;

        if (system.orders.count == system.orders.capacity) {
            resizeOrderContainer(system);
        }
        system.orders.items[system.orders.count] = order;
        system.orders.executed[system.orders.count++] = false;

        executeOrders(system);
    }
    else if (type == Order::Type::SELL) {
        if (sellerUsableCoins(system, walletId) < grnCoins) {
            return false;
        }

        Order order;
        order.type = Order::Type::SELL;
        order.walletId = walletId;
        order.grnCoins = grnCoins;
        if (system.orders.count == system.orders.capacity) {
            resizeOrderContainer(system);
        }
        system.orders.items[system.orders.count] = order;
        system.orders.executed[system.orders.count++] = false; 

        executeOrders(system);
    }

    return true;
}

size_t getFileSize(std::ifstream& file) {
    size_t currentPosition = file.tellg();
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(currentPosition);
    return size;
}

bool quit(const System& system) {
    bool successfullySaved = true;

    std::ofstream walletsFile(WALLETS_FILENAME, std::ios::binary);
    if(walletsFile.is_open()) {
        walletsFile.write((const char*)&system.wallets.items, system.wallets.count * sizeof(Wallet));
        walletsFile.close();
    }
    else {
        successfullySaved = false;
    }

    std::ofstream executedOrdersFile(EXECUTED_ORDERS_FILENAME, std::ios::binary);
    if (executedOrdersFile.is_open()) {
        executedOrdersFile.write((const char*)&system.wallets.executedOrders, system.wallets.count * sizeof(size_t));
        executedOrdersFile.close();
    }
    else {
        successfullySaved = false;
    }

    std::ofstream transactionsFile(TRANSACTIONS_FILENAME, std::ios::binary);
    if (transactionsFile.is_open()) {
        transactionsFile.write((const char*)&system.transactions.items, system.transactions.count * sizeof(Transaction));
        transactionsFile.close();
    }
    else {
        successfullySaved = false;
    }

    std::ofstream ordersFile(ORDERS_FILENAME, std::ios::binary);
    if (ordersFile.is_open()) {
        ordersFile.write((const char*)&system.orders.items, system.orders.count * sizeof(Order));
        ordersFile.close();
    }
    else {
        successfullySaved = false;
    }

    return successfullySaved;
}

void loadSystem(System& system) {
    std::ifstream walletsFile(WALLETS_FILENAME, std::ios::binary);
    if (walletsFile.is_open()) {
        size_t fileSize = getFileSize(walletsFile);
        system.wallets.count = fileSize / sizeof(Wallet);
        system.wallets.capacity = system.wallets.count;
        std::cout << system.wallets.count << std::endl;
        system.wallets.items = new (std::nothrow) Wallet[system.wallets.capacity];
        walletsFile.read((char*)&system.wallets.items, system.wallets.count * sizeof(Wallet));
        walletsFile.close();
    }
    else {
        system.wallets.capacity = INITIAL_CAPACITY;
        system.wallets.count = 0;
        system.wallets.items = new (std::nothrow) Wallet[INITIAL_CAPACITY];
    }

    std::ifstream executedOrdersFile(EXECUTED_ORDERS_FILENAME, std::ios::binary);
    if (executedOrdersFile.is_open()) {
        system.wallets.executedOrders = new (std::nothrow) size_t[system.wallets.capacity];
        walletsFile.read((char*)&system.wallets.executedOrders, system.wallets.count * sizeof(size_t));
        executedOrdersFile.close();
    }
    else {
        system.wallets.executedOrders = new (std::nothrow) size_t[INITIAL_CAPACITY];
    }

    std::ifstream transactionsFile(TRANSACTIONS_FILENAME, std::ios::binary);
    if (transactionsFile.is_open()) {
        size_t fileSize = getFileSize(transactionsFile);
        system.transactions.count = fileSize / sizeof(Transaction);
        system.transactions.capacity = system.transactions.count;
        std::cout << system.transactions.count << std::endl;
        system.transactions.items = new (std::nothrow) Transaction[system.transactions.capacity];
        transactionsFile.read((char*)&system.transactions.items, fileSize);
        transactionsFile.close();
    }
    else {
        system.transactions.capacity = INITIAL_CAPACITY;
        system.transactions.count = 0;
        system.transactions.items = new (std::nothrow) Transaction[INITIAL_CAPACITY];
    }

    std::ifstream ordersFile(ORDERS_FILENAME, std::ios::binary);
    if (ordersFile.is_open()) {
        size_t fileSize = getFileSize(ordersFile);
        system.orders.count = fileSize / sizeof(Order);
        system.orders.capacity = system.orders.count;
        system.orders.items = new (std::nothrow) Order[system.orders.capacity];
        ordersFile.read((char*)&system.orders.items, system.orders.count * sizeof(Order));
        ordersFile.close();
    }
    else {
        system.orders.capacity = INITIAL_CAPACITY;
        system.orders.count = 0;
        system.orders.items = new (std::nothrow) Order[INITIAL_CAPACITY];
    }
    system.orders.executed = new (std::nothrow) bool[system.orders.capacity];
    for (size_t i = 0; i < system.orders.count; i++) {
        system.orders.executed[i] = false;
    }
}

void displayCommands() {
    std::cout << "COMMANDS" << std::endl;
    std::cout << "add-wallet **fiatMoney** **name**" << std::endl;
    std::cout << "make-order **type** **grnCoins** **walletId**" << std::endl;
    std::cout << "transfer **senderId** **receiverId** **grnCoins**" << std::endl;
    std::cout << "wallet-info **walletId**" << std::endl;
    std::cout << "attract-investors" << std::endl;
    std::cout << "quit" << std::endl;
}

int main()
{
    System system;
    loadSystem(system);

    std::cout << "Welcome" << std::endl << std::endl;
    displayCommands();
    std::cout << std::endl;

    char command[MAX_INPUT_LENGTH];
    std::cin >> command;

    bool exit = false;
    while(!exit) {
        if (strcmp(command, "add-wallet")==0) {
            double fiatMoney;
            char name[MAX_INPUT_LENGTH];
            std::cin >> fiatMoney >> name;
            long long result = addWallet(system, fiatMoney, name);
            if (result!=-1) {
                std::cout << "Successfully added wallet with ID " << result << std::endl;
            }
            else {
                std::cout << "Could not add wallet" << std::endl;
            }
        }
        else if (strcmp(command, "make-order")==0) {
            char type[MAX_INPUT_LENGTH];
            double grnCoins;
            unsigned walletId;
            std::cin >> type >> grnCoins >> walletId;
            if (strcmp(type, "buy")==0) {
                if (addOrder(system, walletId, Order::Type::BUY, grnCoins)) {
                    std::cout << "Successfully added order" << std::endl;
                }
                else {
                    std::cout << "Could not add order" << std::endl;
                }
            }
            else if (strcmp(type, "sell")==0) {
                if (addOrder(system, walletId, Order::Type::SELL, grnCoins)) {
                    std::cout << "Successfully added order" << std::endl;
                }
                else {
                    std::cout << "Could not add order" << std::endl;
                }
            }
            else {
                std::cout << "Invalid type of order" << std::endl;
            }
        }
        else if (strcmp(command, "transfer")==0) {
            unsigned senderId, receiverId;
            double grnCoins;
            std::cin >> senderId >> receiverId >> grnCoins;
            if (transfer(system, senderId, receiverId, grnCoins)) {
                std::cout << "Successful transfer" << std::endl;
            }
            else {
                std::cout << "Unsuccessful transfer" << std::endl;
            }
        }
        else if (strcmp(command, "wallet-info")==0) {
            unsigned walletId;
            std::cin >> walletId;
            walletInfo(system, walletId);
        }
        else if (strcmp(command, "attract-investors")==0) {
            attractInvestors(system);
        }
        else if (strcmp(command, "quit")==0) {
            if (quit(system)) {
                std::cout << "Successfully saved data" << std::endl;
            }
            else {
                std::cout << "Could not save data" << std::endl;
            }
            exit = true;
            return 0;
        }

        std::cin >> command;
    }

    return 0;
}