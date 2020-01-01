#include <iostream>
#include <getopt.h>
#include <queue>
#include <sstream>
#include <vector>
#include <map>
#include <string.h>
#include "Stock.h"
#include "Customer.h"

using namespace std;

struct expire_key
{
    int id;
    int expire_time;
    string stock_name;
};

struct expire_compare_t
{
    bool operator()(struct expire_key *a, struct expire_key *b) const
    {
        if (a->expire_time < b->expire_time)
            return true;
        else if (a->expire_time == b->expire_time)
        {
            if (a->id < b->id)
                return true;
        }
        return false;
    }
};


int main(int argc, char *argv[])
{
    std::ios::sync_with_stdio(false);
    std::cin.tie(0);

    // deal with argv
    int c;
    bool verbose = false;
    bool median = false;
    bool midpoint = false;
    bool transfers = false;
    vector<std::string> ttt;

    while (true)
    {
        static struct option long_options[] = {
                {"verbose",   no_argument,       nullptr, 'v'},
                {"median",    no_argument,       nullptr, 'm'},
                {"midpoint",  no_argument,       nullptr, 'p'},
                {"transfers", no_argument,       nullptr, 't'},
                {"ttt",       required_argument, nullptr, 'g'},
                {0, 0,                           0,       0}
        };
        int option_index = 0;
        c = getopt_long(argc, argv, "vmptg:", long_options, &option_index);

        if (c == -1)
            break;
        switch (c)
        {
            case 'v':
                verbose = true;
                break;
            case 'm':
                median = true;
                break;
            case 'p':
                midpoint = true;
                break;
            case 't':
                transfers = true;
                break;
            case 'g':
                ttt.emplace_back(optarg);
                break;
            default:
                abort();
        }
    }

    // start normal things
    add_up end_of_day_count = {0, 0, 0, 0};
    int current_timestamp = 0;
    int id = 0;
    map<expire_key *, stock_key *, expire_compare_t> expired;
    map<string, Stock *> stocks;
    map<string, Customer *> customers;
    string input;
    while (!cin.eof() && getline(cin, input))
    {
        if (input.length() > 0)
        {
            int timestamp;
            string name;
            string behavior;
            string stock_name;
            string read_price;
            string read_quantity;
            int duration;
            stringstream content;
            content.str(input);
            content >> timestamp >> name >> behavior >> stock_name >> read_price >> read_quantity >> duration;
            read_price.erase(0, 1);
            read_quantity.erase(0, 1);
            int price = stoi(read_price);
            int quantity = stoi(read_quantity);

            // start the trades
            auto current_key = new stock_key;
            auto current_value = new stock_value;
            current_key->id = id;
            id++; // hey, this thing is important!
            current_key->price = price;
            current_value->quantity = quantity;

            // check if it is a new customer,if it is, add it
            map<string, Customer *>::iterator name_it;
            name_it = customers.find(name);
            if (name_it == customers.end())
            {
                auto new_customer = new Customer(name);
                customers[name] = new_customer;
            }
            // get this customer now
            current_value->customer = customers[name];

            if (timestamp > current_timestamp)
            {
                // print median
                if (median)
                {
                    //Median match price of EQUITY_SYMBOL at time CURRENT_TIMESTAMP is $MEDIAN_PRICE
                    for (auto it = stocks.begin(); it != stocks.end(); ++it)
                    {
                        int med = it->second->getMedian();
                        if (med != -1)
                            cout << "Median match price of " << it->first << " at time " << current_timestamp << " is $"
                                 << med << endl;
                    }
                }
                // print midpoint
                if (midpoint)
                {
                    // Midpoint of AMD at time 6 is undefined
                    for (auto it = stocks.begin(); it != stocks.end(); ++it)
                    {
                        int mid = it->second->GetMidpoint();
                        if (mid == -1)
                            cout << "Midpoint of " << it->first << " at time " << current_timestamp << " is undefined"
                                 << endl;
                        else
                            cout << "Midpoint of " << it->first << " at time " << current_timestamp << " is $" << mid
                                 << endl;
                    }
                }
                current_timestamp = timestamp;
                // deal with expiration
                int order_time = -1;
                while (true)
                {
                    if (!expired.empty())
                    {
                        auto top = *(expired.begin());
                        order_time = top.first->expire_time;
                        if (order_time > current_timestamp)
                        {
                            break;
                        } else
                        {
                            string top_stock_name = top.first->stock_name;
                            auto top_stock = stocks[top_stock_name];
                            auto erase_order = top.second;
                            top_stock->erase_order(erase_order);
                            expired.erase(top.first);
                            delete top.first;
                        }
                    } else
                        break;
                }
            }

            // check if it is a new stock
            map<string, Stock *>::iterator it;
            it = stocks.find(stock_name);
            // if it is, insert it
            if (it == stocks.end())
            {
                auto current_stock = new Stock();
                stocks[stock_name] = current_stock;
            }

            // deal with the current order
            Stock *curr_stock = stocks[stock_name];
            if (behavior == "BUY")
            {
                if (curr_stock->buy_time != -1 && timestamp >= curr_stock->buy_time)
                {
                    int new_profit =
                            current_key->price - curr_stock->buy_price; //current buy price - ttt's purchase price
                    if (curr_stock->flag || new_profit > curr_stock->profit)
                    {
                        curr_stock->flag=false;
                        curr_stock->profit = new_profit;
                        curr_stock->timestamp2 = timestamp;
                        curr_stock->timestamp1 = curr_stock->buy_time;
                    }
                }

                pair<stock_key *, stock_value *> curr_pair = make_pair(current_key, current_value);
                bool success = true;
                while (success && !curr_stock->IsSellEmpty() && current_value->quantity != 0)
                    success = curr_stock->buyer_trade(&curr_pair, stock_name, &end_of_day_count, verbose);
                if (current_value->quantity != 0 && duration != 0)
                {
                    // if the current sells cannot satisfy this buyer and it is not IOC, push this buyer into the stack
                    curr_stock->AddBuyer(current_key, current_value);
                    if (duration != -1)
                    {
                        // if it is not a permenent order, push it into the expiration map
                        auto curr_exp_key = new expire_key{id, timestamp + duration, stock_name};
                        expired[curr_exp_key] = current_key;
                    }
                } else
                {
                    delete current_key;
                    delete current_value;
                }
            } else
            {
                // when someone is selling, we want to buy sth
                if (curr_stock->buy_time == -1)
                {
                    curr_stock->buy_price = current_key->price;
                    curr_stock->buy_time = timestamp;
                } else if (current_key->price < curr_stock->buy_price)
                {
                    curr_stock->buy_price = current_key->price;
                    curr_stock->buy_time = timestamp;
                }
                pair<stock_key *, stock_value *> curr_pair = make_pair(current_key, current_value);
                bool success = true;
                while (success && !curr_stock->IsBuyerEmpty() && current_value->quantity != 0)
                    success = curr_stock->seller_trade(&curr_pair, stock_name, &end_of_day_count, verbose);
                if (current_value->quantity != 0)
                {
                    // if the current sell is not sold out, push it into the stack
                    if (duration != 0)
                    {
                        curr_stock->AddSeller(current_key, current_value);
                        if (duration != -1)
                        {
                            // if it is not a permenent order, push it into the expiration map
                            auto curr_exp_key = new expire_key{id, timestamp + duration, stock_name};
                            expired[curr_exp_key] = current_key;
                        }
                    }
                }
                else
                {
                    delete current_key;
                    delete current_value;
                }
            }
        } else
            break;
    }

    // print mid and med at the end
    if (median)
    {
        //Median match price of EQUITY_SYMBOL at time CURRENT_TIMESTAMP is $MEDIAN_PRICE
        for (auto it = stocks.begin(); it != stocks.end(); ++it)
        {
            int med = it->second->getMedian();
            if (med != -1)
                cout << "Median match price of " << it->first << " at time " << current_timestamp << " is $" << med
                     << endl;
        }
    }
    // print midpoint
    if (midpoint)
    {
        // Midpoint of AMD at time 6 is undefined
        for (auto it = stocks.begin(); it != stocks.end(); ++it)
        {
            int mid = it->second->GetMidpoint();
            if (mid == -1)
                cout << "Midpoint of " << it->first << " at time " << current_timestamp << " is undefined" << endl;
            else
                cout << "Midpoint of " << it->first << " at time " << current_timestamp << " is $" << mid << endl;
        }
    }
    // end of day cases
    cout << "---End of Day---" << endl;
    cout << "Commission Earnings: $" << end_of_day_count.commision_earning << endl;
    cout << "Total Amount of Money Transferred: $" << end_of_day_count.money_transferred << endl;
    cout << "Number of Completed Trades: " << end_of_day_count.num_of_trades << endl;
    cout << "Number of Shares Traded: " << end_of_day_count.num_of_shares_traded << endl;
    if (transfers)
    {
        for (auto it = customers.begin(); it != customers.end(); ++it)
        {
            it->second->PrintTransfers();
        }
    }

    // ttt
    if (!ttt.empty())
    {
        for (auto it = ttt.begin(); it != ttt.end(); ++it)
        {

            //Time travelers would buy EQUITY_SYMBOL at time: TIMESTAMP1 and sell it at time: TIMESTAMP2
            cout << "Time travelers would buy " << *it << " at time: " << stocks[*it]->timestamp1
                 << " and sell it at time: " << stocks[*it]->timestamp2 << endl;
        }
    }

    for (auto it = customers.begin(); it != customers.end(); ++it)
    {
        delete it->second;
    }
    for (auto it = stocks.begin(); it != stocks.end(); ++it)
    {
        delete it->second;
    }
    for (auto it = expired.begin(); it != expired.end(); ++it)
    {
        delete it->first;
        //delete it->second;
    }

    return 0;
}

