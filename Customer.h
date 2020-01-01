//
// Created by zhang on 2018/11/27.
//

#ifndef P4_CUSTOMER_H
#define P4_CUSTOMER_H

#include <iostream>

using namespace std;

class Customer
{
private:
    string name;
    int num_of_stocks_bought;
    int num_of_stocks_sold;
    int net_value;

public:
    Customer(string name)
    {
        this->name = name;
        this->num_of_stocks_bought = this->num_of_stocks_sold = this->net_value = 0;
    }

    string GetName()
    {
        return this->name;
    }

    void BuyTrade(int quantity, int price)
    {
        this->num_of_stocks_bought+=quantity;
        this->net_value -= quantity * price;
    }

    void SellTrade(int quantity, int price)
    {
        this->num_of_stocks_sold+=quantity;
        this->net_value += quantity * price;
    }

    void PrintTransfers()
    {
        //CLIENT_NAME bought NUMBER_OF_STOCKS_BOUGHT and sold NUMBER_OF_STOCKS_SOLD for a net transfer of $NET_VALUE_TRADED
        cout << this->name << " bought " << this->num_of_stocks_bought << " and sold " << this->num_of_stocks_sold
             << " for a net transfer of $" << this->net_value << endl;
    }

    ~Customer()
    {

    }
};

#endif //P4_CUSTOMER_H
