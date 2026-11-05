#include <iostream>
#include<list>
#include<algorithm>
using namespace std;

const int BUY = 0;
const int SELL = 1;
class Order{
    public:
    int orderId;
    int side;
    double price;
    int quantity;
    long long timestamp;
    Order(){}
};

class Pricelevel{
    public:
    double price;
    int totalVolume;
    list<Order> orders;
    Pricelevel* left;
    Pricelevel* right;
    int height;

    Pricelevel(double p){
        totalVolume = 0;
        left = nullptr;
        right = nullptr;
        height = 1;
        price = p;
    }
};

class PriceBookAVL{
    private:
    Pricelevel* root = nullptr;
    public:
    friend class OrderBook;
    PriceBookAVL(){
        root = nullptr;
    }

    int getHeight(Pricelevel* node){
        if(node == nullptr) return 0;

        return node->height;
    }

    int getBalanceFactor(Pricelevel* node){
        if(node==nullptr) return 0;

        return getHeight(node->left)-getHeight(node->right);
    }

    void updateHeight(Pricelevel* node){
        node->height = 1+ max(getHeight(node->left),getHeight(node->right));
    }

    Pricelevel* rightRotate(Pricelevel* y){
        Pricelevel* x = y->left;
        Pricelevel* t2 = x->right;
        x->right = y;
        y->left = t2;
        updateHeight(y);
        updateHeight(x);
        return x;
    }

    Pricelevel* leftRotate(Pricelevel* x){
        Pricelevel* y = x->right;
        Pricelevel* t2 = y->left;
        y->left = x;
        x->right = t2;
        updateHeight(x);
        updateHeight(y);
        return y;
    }

    Pricelevel* insertHelper(Order& ord, Pricelevel* node){
        if(node == nullptr){
            Pricelevel* newNode = new Pricelevel(ord.price);
            newNode->orders.push_back(ord);
            newNode->totalVolume += ord.quantity;
            return newNode;
        }
        if(ord.price < node->price){
            node->left = insertHelper(ord,node->left);
        }
        if(ord.price > node->price){
            node->right = insertHelper(ord,node->right);
        }
        if(node->price == ord.price){
            node->orders.push_back(ord);
            node->totalVolume += ord.quantity;
            return node;
        }

        updateHeight(node);
        int bf = getBalanceFactor(node);

        if(bf > 1 && ord.price < node->left->price){
            return rightRotate(node);
        }

        if(bf<-1 && ord.price > node->right->price){
            return leftRotate(node);
        }

        if(bf>1 && ord.price > node->left->price){
            node->left = leftRotate(node);
            return rightRotate(node);
        }

        if(bf < -1 && ord.price < node->right->price){
            node->right = rightRotate(node);
            return leftRotate(node);
        }

        return node;
    }
    void insertOrder(Order& ord){
        root = insertHelper(ord,root);
    }

    void displayInOrder(Pricelevel* node){
        if(node ==  nullptr) return;

        displayInOrder(node->left);
        cout<<"Price : "<<node->price<<" Rs. | Total Volume "<< node->totalVolume << " shares"<<endl;
        displayInOrder(node->right);
    }

    void printBook(){
        cout<<"Current Order Book :-"<<endl;
        displayInOrder(root);
    }

    int queryRangeHelper(Pricelevel* node, double lowPrice, double highPrice){
        if(node == nullptr) return 0;
        if(node->price > highPrice){
            return queryRangeHelper(node->left,lowPrice,highPrice);
        }
        if(node->price < lowPrice){
            return queryRangeHelper(node->right,lowPrice,highPrice);
        }
        return node->totalVolume + queryRangeHelper(node->left,lowPrice,highPrice) + queryRangeHelper(node->right,lowPrice,highPrice);
    }

    long long getVolumeInRange(double lowPrice, double HighPrice){
        return queryRangeHelper(root,lowPrice,HighPrice);
    }
};

class OrderBook{
    public:
    PriceBookAVL bids;
    PriceBookAVL asks;

    Pricelevel* getMinAsk(Pricelevel* node){
        if(node == nullptr) return nullptr;
        while(node->left != nullptr){
            node = node->left;
        }
        return node;
    }

    Pricelevel* getMaxBid(Pricelevel* node){
        if(node == nullptr) return nullptr;
        while(node->right != nullptr){
            node = node->right;
        }
        return node;
    }

    void processOrder(Order& ord){
        if(ord.side == BUY){
            while(ord.quantity > 0 && asks.root != nullptr){
                Pricelevel* bestAsk = getMinAsk(asks.root);
            if(ord.price < bestAsk->price){
                break;
            }
            Order& restingOrder = bestAsk->orders.front();

            if(ord.quantity >= restingOrder.quantity){
                ord.quantity -= restingOrder.quantity;
                cout << "TRADE EXECUTED! Matched " << restingOrder.quantity << " shares at " << bestAsk->price << " Rs" << endl;

                bestAsk->totalVolume -= restingOrder.quantity;
                bestAsk->orders.pop_front();

                if(bestAsk->orders.empty()){
                    //to be done
                }
            }
            else{
                restingOrder.quantity -= ord.quantity;
                bestAsk->totalVolume -= ord.quantity;
                cout << "TRADE EXECUTED! Matched " << ord.quantity << " shares at " << bestAsk->price << " Rs" << endl;
                ord.quantity = 0;
                break;
            }
        }

        if(ord.quantity > 0){
            bids.insertOrder(ord);
            cout << "Order " << ord.orderId << " added to Bids book (Resting)." << endl;
        }
    }
    else{
        while (ord.quantity > 0 && bids.root != nullptr) {
                Pricelevel* bestBid = getMaxBid(bids.root);
                if (ord.price > bestBid->price) {
                    break;
                }
                Order& restingOrder = bestBid->orders.front();

                if (ord.quantity >= restingOrder.quantity) {
                    ord.quantity -= restingOrder.quantity;
                    cout << "TRADE EXECUTED! Matched " << restingOrder.quantity << " shares at " << bestBid->price << " Rs" << endl;
                    bestBid->totalVolume -= restingOrder.quantity;
                    bestBid->orders.pop_front();
                } else {
                    restingOrder.quantity -= ord.quantity;
                    bestBid->totalVolume -= ord.quantity;
                    cout << "TRADE EXECUTED! Matched " << ord.quantity << " shares at " << bestBid->price << " Rs" << endl;
                    ord.quantity = 0;
                    break;
                }
            }

            if (ord.quantity > 0) {
                asks.insertOrder(ord);
                cout << "Order " << ord.orderId << " added to Asks book (Resting)." << endl;
            }
    }
}
};

int main(){
  
}