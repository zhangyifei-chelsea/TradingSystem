//
// Created by zhang on 2018/11/26.
//

#ifndef P4_STOCK_H
#define P4_STOCK_H

#include <iostream>
#include <queue>
#include <string.h>
#include <map>
#include "Customer.h"

using namespace std;

struct add_up{
    int commision_earning;
    int money_transferred;
    int num_of_trades;
    int num_of_shares_traded;
};

struct orders
{
    int timestamp;
    Customer *name;
    string behavoir;
    int price;
    int quantity;
    int duration;
};

struct stock_key
{
    int id;
    int price;
};

struct stock_value
{
    Customer *customer;
    int quantity;
    //int expire_time;
};


class Stock
{
private:

    struct seller_compare_t
    {
        bool operator()(struct stock_key *a, struct stock_key *b) const
        {
            if (a->price < b->price)
                return true;
            else if (a->price == b->price)
            {
                if (a->id < b->id)
                    return true;
            }
            return false;
        }
    };

    struct buyer_compare_t
    {
        bool operator()(struct stock_key *a, struct stock_key *b) const
        {
            if (a->price > b->price)
                return true;
            else if (a->price == b->price)
            {
                if (a->id < b->id)
                    return true;
            }
            return false;
        }
    };

    priority_queue<int, vector<int>, std::greater<int>> min_heap;
    priority_queue<int, vector<int>, std::less<int>> max_heap;
    map<stock_key *, stock_value *, seller_compare_t> seller_stack;
    map<stock_key *, stock_value *, buyer_compare_t> buyer_stack;
public:

    // supporting ttt
    int timestamp1=-1,timestamp2=-1;
    int buy_price;
    int buy_time=-1;
    int profit=-1;
    bool flag=true;

    Stock()
    {

    }

    ~Stock()
    {
        for (auto it = seller_stack.begin(); it != seller_stack.end(); ++it)
        {
            delete it->first;
            delete it->second;
        }
    }

    void AddBuyer(stock_key *key, stock_value *value)
    {
        buyer_stack[key] = value;
    }


    void AddSeller(stock_key *key, stock_value *value)
    {
        seller_stack[key] = value;
    }


    pair<stock_key *, stock_value *> GetTopSeller()
    {
        return *(seller_stack.begin());
    }

    pair<stock_key *, stock_value *> GetTopBuyer()
    {
        return *(buyer_stack.begin());
    }

    bool IsSellEmpty()
    {
        return seller_stack.empty();
    }

    bool IsBuyerEmpty()
    {
        return buyer_stack.empty();
    }

    bool buyer_trade(pair<stock_key *, stock_value *> *buy_order, string stock_name, add_up *end_of_day, bool verbose)
    {
        // if buyer's price > seller's price, we can start the deal
        // if any of the quantity is 0, we need to remove them and also (delete)
        // if the deal fails, it means there will be no more possible deals in this round
        // if it return false, there is no need to continue solving the deal, and we need to push the deal into the stack
        // there are mainly two cases, if the buy_order is satisfied, its quantity becomes 0, or the it can no longer be satisfied in this timestamp;
        auto sell_order = GetTopSeller();
        if (!sell_order.first)
            return false;
        if (sell_order.first->price <= buy_order->first->price)
        {
            // the trade is done in the price of the seller, since seller comes first
            int trade_price = sell_order.first->price;
            int trade_quantity;
            if (sell_order.second->quantity <= buy_order->second->quantity)
            {
                trade_quantity = sell_order.second->quantity;
                // in this case, the sell_order will be totally consumed.
                buy_order->second->quantity -= trade_quantity;
                seller_stack.erase(sell_order.first);
                //delete sell_order.first;
                //delete sell_order.second;
            } else
            {
                trade_quantity = buy_order->second->quantity;
                sell_order.second->quantity -= trade_quantity;
                buy_order->second->quantity = 0;
            }
            end_of_day->num_of_trades++;
            end_of_day->money_transferred += trade_quantity * trade_price;
            end_of_day->num_of_shares_traded += trade_quantity;
            end_of_day->commision_earning += 2 * int(trade_quantity * trade_price/100);
            sell_order.second->customer->SellTrade(trade_quantity, trade_price);
            buy_order->second->customer->BuyTrade(trade_quantity, trade_price);

            if (verbose)
                cout << buy_order->second->customer->GetName() << " purchased " << trade_quantity << " shares of "
                     << stock_name << " from " << sell_order.second->customer->GetName() << " for $" << trade_price
                     << "/share" << endl;

            // insert price into heap.
            insertMedian(trade_price);
            return true;
        } else
            return false;
    }

    bool seller_trade(pair<stock_key *, stock_value *> *sell_order, string stock_name, add_up *end_of_day, bool verbose)
    {
        // if buyer's price > seller's price, we can start the deal
        // if any of the quantity is 0, we need to remove them and also (delete)
        // if the deal fails, it means there will be no more possible deals in this round
        // if it return false, there is no need to continue solving the deal, and we need to push the deal into the stack
        // there are mainly two cases, if the buy_order is satisfied, its quantity becomes 0, or the it can no longer be satisfied in this timestamp;
        auto buy_order = GetTopBuyer();
        if (!buy_order.first)
            return false;
        if (sell_order->first->price <= buy_order.first->price)
        {
            // the trade is done in the price of the buyer, since buyer comes first
            int trade_price = buy_order.first->price;
            int trade_quantity;
            if (sell_order->second->quantity < buy_order.second->quantity)
            {
                trade_quantity = sell_order->second->quantity;

                // in this case, the sell_order will be totally consumed.
                sell_order->second->quantity = 0;
                buy_order.second->quantity -= trade_quantity;
            } else
            {
                trade_quantity = buy_order.second->quantity;

                sell_order->second->quantity -= trade_quantity;
                //auto need_delete=buy_order.second;
                //delete buy_order.second;
                buyer_stack.erase(buy_order.first);
                //delete buy_order.first;
                //delete buy_order.second;
            }
            end_of_day->num_of_trades++;
            end_of_day->money_transferred += trade_quantity * trade_price;
            end_of_day->num_of_shares_traded += trade_quantity;
            end_of_day->commision_earning += 2 * int(trade_quantity * trade_price/100);
            if (verbose)
                cout << buy_order.second->customer->GetName() << " purchased " << trade_quantity << " shares of "
                     << stock_name << " from " << sell_order->second->customer->GetName() << " for $" << trade_price
                     << "/share" << endl;
            sell_order->second->customer->SellTrade(trade_quantity, trade_price);
            buy_order.second->customer->BuyTrade(trade_quantity, trade_price);


            // insert price into heap.
            insertMedian(trade_price);
            return true;
        } else
            return false;
    }

    void erase_order(stock_key *key)
    {
        if (seller_stack.find(key) != seller_stack.end())
        {
            auto value = seller_stack[key];
            delete value;
            seller_stack.erase(key);
            delete key;
        } else
        {
            auto value = buyer_stack[key];
            delete value;
            buyer_stack.erase(key);
            delete key;
        }
    }

    int GetMidpoint()
    {
        if (IsSellEmpty() || IsBuyerEmpty())
            return -1;
        else
        {
            auto highest_buy = GetTopBuyer();
            auto lowest_sell = GetTopSeller();
            return (highest_buy.first->price + lowest_sell.first->price) / 2;
        }
    }

    void insertMedian(int price)
    {
        int size = min_heap.size() + max_heap.size();
        if(size==0)
            max_heap.push(price);
        else
        {
            if(size%2)
            {
                // before insertion it is odd
                if(price>=max_heap.top())
                {
                    min_heap.push(price);
                }
                else
                {
                    int max=max_heap.top();
                    max_heap.pop();
                    min_heap.push(max);
                    max_heap.push(price);
                }
            }
            else
            {
                if(price<=min_heap.top())
                {
                    max_heap.push(price);
                }
                else
                {
                    int min=min_heap.top();
                    min_heap.pop();
                    max_heap.push(min);
                    min_heap.push(price);
                }
            }
        }
    }


    int getMedian()
    {
        int size = max_heap.size()+min_heap.size();
        if(size==0)
            return -1;
        else
        {
            if(size%2)
            {
                return max_heap.top();
            }
            else
            {
                int med=(max_heap.top()+min_heap.top())/2;
                return med;
            }
        }
    }
};

#endif //P4_STOCK_H
