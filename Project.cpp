#include <iostream>
#include <cstring>
#include <fstream>
#include <ctime>

const int SYSTEM_WALLET_ID = 4294967295;
const size_t INITIAL_CAPACITY = 2;
const int EXCHANGE_RATE = 375;
const int RICHEST_USERS_COUNT = 10;

const char* COMMANDS[1024] = {
    "List commands",
    "Create wallet",
    "Add order",
    "Transfer",
    "Quit"
};

const char WALLETS_FILENAME[] = "wallets.dat";
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

bool addWallet(System& system, const double fiatMoney, const char name[]) {
    if (strlen(name) <= 255) {
        Wallet wallet;
        strcpy(wallet.owner, name);
        wallet.id = generateId();
        wallet.fiatMoney = fiatMoney;

        if (system.wallets.count == system.wallets.capacity) {
            resizeWalletContainer(system);
        }
        system.wallets.items[system.wallets.count++] = wallet;

        if (transfer(system, SYSTEM_WALLET_ID, wallet.id, wallet.fiatMoney / EXCHANGE_RATE)) {
            return true;
        }
    }
    return false;
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

void attractInvestors(System& system) {
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

bool transfer(System& system, const unsigned senderId, const unsigned receiverId, const double grnCoins) {
    if (findWallet(system, senderId) == nullptr ||
        findWallet(system, receiverId) == nullptr ||
        getCoins(system, senderId) < grnCoins) {
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

void resizeWalletContainer(System& system) {
    Wallet* newWallets = new (std::nothrow) Wallet[system.wallets.capacity *= 2];
    for (size_t i = 0; i < system.wallets.count; i++) {
        newWallets[i] = system.wallets.items[i];
    }
    delete[] system.wallets.items;
    system.wallets.items = newWallets;
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

void quit(const System& system) {
    std::ofstream walletsFile(WALLETS_FILENAME, std::ios::binary);
    walletsFile.write((const char*)&system.wallets.count, sizeof(size_t));
    walletsFile.write((const char*)&system.wallets.items, system.wallets.count * sizeof(Wallet));
    walletsFile.close();

    std::ofstream transactionsFile(TRANSACTIONS_FILENAME, std::ios::binary);
    walletsFile.write((const char*)&system.transactions.count, sizeof(size_t));
    transactionsFile.write((const char*)&system.transactions.items, system.transactions.count * sizeof(Transaction));
    transactionsFile.close();

    std::ofstream ordersFile(ORDERS_FILENAME, std::ios::binary);
    ordersFile.write((const char*)&system.orders.count, sizeof(size_t));
    ordersFile.write((const char*)&system.orders.items, system.orders.count * sizeof(Order));
    ordersFile.close();
}

void loadSystem(System& system) {
    std::ifstream walletsFile(WALLETS_FILENAME, std::ios::binary);
    if (walletsFile.is_open()) {
        walletsFile.read((char*)&system.wallets.count, sizeof(size_t));
        system.wallets.capacity = system.wallets.count;
        system.wallets.items = new (std::nothrow) Wallet[system.wallets.capacity];
        walletsFile.read((char*)&system.wallets.items, system.wallets.count * sizeof(Wallet));
        walletsFile.close();
    }
    else {
        system.wallets.capacity = INITIAL_CAPACITY;
        system.wallets.count = 0;
        system.wallets.items = new (std::nothrow) Wallet[INITIAL_CAPACITY];
    }

    std::ifstream transactionsFile(TRANSACTIONS_FILENAME, std::ios::binary);
    if (transactionsFile.is_open()) {
        transactionsFile.read((char*)&system.transactions.count, sizeof(size_t));
        system.transactions.capacity = system.transactions.count;
        system.transactions.items = new (std::nothrow) Transaction[system.transactions.capacity];
        transactionsFile.read((char*)&system.transactions.items, system.transactions.count * sizeof(Transaction));
        transactionsFile.close();
    }
    else {
        system.transactions.capacity = INITIAL_CAPACITY;
        system.transactions.count = 0;
        system.transactions.items = new (std::nothrow) Transaction[INITIAL_CAPACITY];
    }

    std::ifstream ordersFile(ORDERS_FILENAME, std::ios::binary);
    if (ordersFile.is_open()) {
        ordersFile.read((char*)&system.orders.count, sizeof(size_t));
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
}

int main()
{
    System system;
    loadSystem(system);



    return 0;
}