#ifndef MINISQL_BPLUS_TREE_H
#define MINISQL_BPLUS_TREE_H

#include <iostream>
#include <string>
#include <cstring>
#include <vector>

#include "Minisql.h"
#include "BufferManager.h"

using namespace std;

static BufferManager bufferMa;

struct KeyInfo {   //Find a key in the node.
    int position;
    bool ifExist;
};

//The node of B+ tree
template<class Type>
class Node {
public:
    int keycount;
    int degree;
    bool leaf;
    Node *parent;
    Node *next;
    vector<Node *> childs;
    vector<Type> keys;
    vector<int> vals;
    //  friend class BPlusTree;
public:
    Node(int _degree, bool ifLeaf = false);

    ~Node();

    bool IsRoot();

    void FindKey(Type keyvalue, KeyInfo &keySearch);

    bool AddKey(Type &keyvalue, Node *child);

    bool AddKey(Type &keyvalue, int val);

    bool RemoveKey(Type &keyvalue);

    Node *Split();

    void PrintNode();
};

//B+ tree
template<class Type>
class BPlusTree {
private:
    typedef Node<Type> BNode;
    struct NodeInfo {
        BNode *n;
        int position;
        bool ifExist;
    };
    BNode *root;
    BNode *firstLeaf;
    int degree;
    int level;
    int keycount;
    int nodecount;
    int keysize;
    string fileName;
    fileNode *file;

    void InsertAdjust(BNode *curNode);

    int GetChildPos(BNode *parent, BNode *child);

    void DeleteAdjust(BNode *curNode);

public:
    BPlusTree(string fName, int _size, int _degree);

    ~BPlusTree();

    void BuildTree();

    void DropTree(BNode *curNode);

    void FindNode(Type keyvalue, NodeInfo &nodeSearch);

    bool InsertKey(Type &keyvalue, int vals);

    int SearchKey(Type keyvalue);

    bool DeleteKey(Type &keyvalue);

    void ReadFromDiskAll();

    void WriteBackToDiskAll();

    void ReadFromDisk(blockNode *btmp);

    void PrintTreeNode(BNode *curNode);

    void PrintTree();
};

//constructor
template<class Type>
Node<Type>::Node(int _degree, bool ifLeaf):degree(_degree), leaf(ifLeaf) {
    int i;
    keycount = 0;
    parent = next = NULL;
    Type ini_value;
    for (i = 0; i < degree + 1; i++) {
        childs.push_back(NULL);
        vals.push_back(0);
        keys.push_back(ini_value);
    }
    childs.push_back(NULL);
}

//destructor
template<class Type>
Node<Type>::~Node() {

}

//Judge whether the node is the root.
template<class Type>
bool Node<Type>::IsRoot() {
    return (parent == NULL);
}

//Find the position of the key in the node
template<class Type>
void Node<Type>::FindKey(Type keyvalue, KeyInfo &keySearch) {
    int i;
    if (!keycount) {   //There is no key in the node.
        keySearch.position = 0;
        keySearch.ifExist = false;
        return;
    }
    for (i = 0; i < keycount && keys[i] < keyvalue; i++);
    if (i == keycount) { //All keys are smaller than the target
        keySearch.position = keycount;
        keySearch.ifExist = false;
    } else if (keys[i] == keyvalue) {
        keySearch.position = i;
        keySearch.ifExist = true;
    } else if (keys[i] > keyvalue) {
        keySearch.position = i;
        keySearch.ifExist = false;
    }
    return;
}

//The function is for none-leaf node.
//Add a new key value into the node and insert the child into its right pointer.
template<class Type>
bool Node<Type>::AddKey(Type &keyvalue, Node<Type> *child) {
    int i;
    if (leaf) {
        cout << "Error: AddKey(Type &keyvalue) can only be called by nonleaf nodes." << endl;
        return false;
    }
    if (!keycount) {
        keys[0] = keyvalue;
        childs[1] = child;
        if (child)
            child->parent = this;
        keycount = 1;
        return true;
    }
    struct KeyInfo keySearch;
    FindKey(keyvalue, keySearch);
    if (keySearch.ifExist) {  //The key already exists.
        cout << "Error: In AddKey(Type &keyvalue), the key has already exists." << endl;
        return false;
    } else {  //add the key and the child
        for (i = keycount; i > keySearch.position; i--) {
            keys[i] = keys[i - 1];
            childs[i + 1] = childs[i];
        }
        childs[keySearch.position + 1] = child;
        if (child)
            child->parent = this;
        keys[keySearch.position] = keyvalue;
        keycount++;
        return true;
    }
}

//The function is for leaf-node
//Add a new key value into the node.
template<class Type>
bool Node<Type>::AddKey(Type &keyvalue, int val) {
    int i;
    if (!leaf) {
        cout << "Error: AddKey(Type &keyvalue, int val) can only be called by leaf nodes." << endl;
        return false;
    }
    if (!keycount) {
        keys[0] = keyvalue;
        vals[0] = val;
        keycount = 1;
        return true;
    }
    struct KeyInfo keySearch;
    FindKey(keyvalue, keySearch);
    if (keySearch.ifExist) {  //The key already exists.
        cout << "Error: In AddKey(Type &keyvalue, int val), the key has already exists." << endl;
        return false;
    } else {  //add the key and the val.
        for (i = keycount; i > keySearch.position; i--) {
            keys[i] = keys[i - 1];
            vals[i] = vals[i - 1];
        }
        keys[keySearch.position] = keyvalue;
        vals[keySearch.position] = val;
        keycount++;
        return true;
    }
}

//delete a keyvalue in the node and delete its right pointer
template<class Type>
bool Node<Type>::RemoveKey(Type &keyvalue) {
    static Type ini_value;
    int i;
    struct KeyInfo keySearch;
    FindKey(keyvalue, keySearch);
    if (!keySearch.ifExist) {  //the key doesn't exist.
        cout << "Error: In Remove(Type &keyvalue), the key to remove is not found." << endl;
        return false;
    }
    if (leaf) {  //delete the val with the key
        for (i = keySearch.position; i < keycount - 1; i++) {
            keys[i] = keys[i + 1];
            vals[i] = vals[i + 1];
        }
        keys[keycount - 1] = ini_value;
        vals[keycount - 1] = 0;
        keycount--;
    } else {  //delete the child node with the key
        for (i = keySearch.position + 1; i < keycount; i++) {
            keys[i - 1] = keys[i];
            childs[i] = childs[i + 1];
        }
        keys[keycount - 1] = ini_value;
        childs[keycount] = NULL;
        keycount--;
    }
    return true;
}

//Split the node into two nodes.
template<class Type>
Node<Type> *Node<Type>::Split() {
    static Type ini_value;
    int pos = (degree + 1) / 2, i;  //the position to split
    Node<Type> *newNode = new Node<Type>(degree, this->leaf);
    if (leaf) {  //For leaf node, all keys will split into the two nodes
        for (i = pos; i < degree; i++) {
            newNode->keys[i - pos] = this->keys[i];
            this->keys[i] = ini_value;
            newNode->vals[i - pos] = this->vals[i];
            this->vals[i] = 0;
        }
        newNode->parent = this->parent;
        newNode->keycount = pos - 1;
        this->keycount = pos;
        newNode->next = this->next;
        this->next = newNode;
    } else {  //for non-leaf node, the middle key will be placed into the parent node.
        for (i = pos; i < degree; i++) {
            newNode->keys[i - pos] = this->keys[i];
            this->keys[i] = Type();
            newNode->childs[i - pos] = this->childs[i];
            newNode->childs[i - pos]->parent = newNode;
            this->childs[i] = NULL;
        }
        newNode->childs[(degree - 1) / 2] = this->childs[degree];
        newNode->childs[(degree - 1) / 2]->parent = newNode;
        this->childs[degree] = NULL;
        newNode->parent = this->parent;
        newNode->keycount = pos - 1;
        this->keycount = pos - 1;
    }
    return newNode;
}

//Print the information of the node. 
template<class Type>
void Node<Type>::PrintNode() {
    int i;
    cout << "#######################DEBUG Node Print#######################" << endl;
    cout << "Address: " << (void *) this << ", ";
    cout << "KeyCount: " << keycount << ", ";
    cout << "Parent: " << (void *) parent << ", ";
    if (leaf) {
        cout << "IsLeaf: " << "Yes, " << "NextLeaf: " << (void *) next << endl;
    } else {
        cout << "IsLeaf: " << "No" << endl;
    }
    cout << "Keys: {" << keys[0];
    for (i = 1; i < keycount; i++)
        cout << ", " << keys[i];
    cout << "}" << endl;
    if (leaf) {
        cout << "Vals: {" << vals[0];
        for (i = 1; i < keycount; i++)
            cout << ", " << vals[i];
        cout << "}" << endl;
    } else {
        cout << "Childs: {" << (void *) childs[0];
        for (i = 1; i <= keycount; i++)
            cout << ", " << (void *) childs[i];
        cout << "}" << endl;
    }
    cout << "##############################################################" << endl;
}

//constructor
template<class Type>
BPlusTree<Type>::BPlusTree(string fName, int _size, int _degree) {
    fileName = fName;
    keysize = _size;
    degree = _degree;
    keycount = nodecount = level = 0;
    root = firstLeaf = NULL;
    file = NULL;
    BuildTree();
    ReadFromDiskAll();
}

//destructor
template<class Type>
BPlusTree<Type>::~BPlusTree() {
    DropTree(root);
    level = nodecount = keycount = 0;
    root = firstLeaf = NULL;
}

//Build a new B+ Tree.
template<class Type>
void BPlusTree<Type>::BuildTree() {
    root = new Node<Type>(degree, true);
    keycount = 0;
    level = 1;
    nodecount = 1;
    firstLeaf = root;
}

//Drop a B+ Tree.
template<class Type>
void BPlusTree<Type>::DropTree(BNode *curNode) {
    int i;
    if (curNode == NULL)
        return;
    if (curNode->leaf) {
        delete curNode;
        nodecount--;
        return;
    }
    for (i = 0; i <= curNode->keycount; i++) {
        DropTree(curNode->childs[i]);
        curNode->childs[i] = NULL;
    }
    delete curNode;
    nodecount--;
    return;
}

//Find the leaf-node containing the target key.
template<class Type>
void BPlusTree<Type>::FindNode(Type keyvalue, NodeInfo &nodeSearch) {
    struct KeyInfo keySearch;
    if (!root) {  //the root is null
        nodeSearch.n = NULL;
        nodeSearch.position = 0;
        nodeSearch.ifExist = false;
        return;
    }
    Node<Type> *curNode = root;
    while (!curNode->leaf) {  //get to the leaf node
        curNode->FindKey(keyvalue, keySearch);
        if (keySearch.ifExist)
            curNode = curNode->childs[keySearch.position + 1];
        else
            curNode = curNode->childs[keySearch.position];
    }
    curNode->FindKey(keyvalue, keySearch);
    nodeSearch.n = curNode;
    nodeSearch.position = keySearch.position;
    nodeSearch.ifExist = keySearch.ifExist;
    return;
}

//Insert a new key into the B+ tree.
template<class Type>
bool BPlusTree<Type>::InsertKey(Type &keyvalue, int val) {
    struct NodeInfo nodeSearch;
    struct KeyInfo oriSearch;
    Type oriValue;
    if (!root) BuildTree();
    if (root->leaf && root->keycount == 0) {
        root->keycount = 1;
        root->keys[0] = keyvalue;
        root->vals[0] = val;
        keycount = 1;
        return true;
    }
    FindNode(keyvalue, nodeSearch);  //find the place to insert.
    if (nodeSearch.ifExist) {
        cout << "Error: the key has already existed." << endl;
        return false;
    }
    oriValue = nodeSearch.n->keys[0];
    nodeSearch.n->AddKey(keyvalue, val);
    keycount++;
    //Update the none-leaf node if the new key is at the first position in the leaf node.
    if (nodeSearch.n->keys[0] == keyvalue) {
        BNode *curNode = nodeSearch.n->parent;
        while (curNode) {
            curNode->FindKey(oriValue, oriSearch);
            if (oriSearch.ifExist) {
                curNode->keys[oriSearch.position] = keyvalue;
                break;
            } else {
                curNode = curNode->parent;
            }
        }
    }
    if (nodeSearch.n->keycount == degree) {  //adjust the B+ tree.
        InsertAdjust(nodeSearch.n);
    }
    return true;
}

//Adjust the B+ tree after inserting.
template<class Type>
void BPlusTree<Type>::InsertAdjust(BNode *curNode) {
    Type keyvalue;
    BNode *parentNode;
    while (curNode->keycount == degree) {
        if (curNode->leaf) keyvalue = curNode->keys[(degree + 1) / 2];
        else keyvalue = curNode->keys[(degree - 1) / 2];
        BNode *newNode = curNode->Split();
        nodecount++;
        if (curNode->parent == NULL) {   //If the node splited is the root node.
            BNode *rootNode = new Node<Type>(degree, false);
            level++;
            nodecount++;
            rootNode->keycount = 1;
            rootNode->keys[0] = keyvalue;
            rootNode->childs[0] = curNode;
            rootNode->childs[1] = newNode;
            curNode->parent = rootNode;
            newNode->parent = rootNode;
            this->root = rootNode;
            return;
        }
        //Change the key and child of the parent node.
        parentNode = curNode->parent;
        parentNode->AddKey(keyvalue, newNode);
        newNode->parent = parentNode;
        //split the parent if necessary.
        curNode = parentNode;
    }
}

//search the key in the B+ Tree.
template<class Type>
int BPlusTree<Type>::SearchKey(Type keyvalue) {
    if (root == NULL)
        return -1;
    struct NodeInfo nodeSearch;
    FindNode(keyvalue, nodeSearch);
    if (!nodeSearch.ifExist)
        return -1;
    else  //return the val.
        return nodeSearch.n->vals[nodeSearch.position];
}

//Delete a key in the B+ Tree.
template<class Type>
bool BPlusTree<Type>::DeleteKey(Type &keyvalue) {
    struct NodeInfo nodeSearch;
    BNode *parentNode;
    if (!root) {
        cout << "Error: No nodes are in the BPlus Tree." << endl;
        return false;
    }
    FindNode(keyvalue, nodeSearch);
    if (!nodeSearch.ifExist) {
        cout << "Error: the key to delete is not in the tree." << endl;
        return false;
    }
    if (nodeSearch.n->parent == NULL) {  //the root node is also the leaf node
        nodeSearch.n->RemoveKey(keyvalue);
        keycount--;
        DeleteAdjust(nodeSearch.n);
    } else {
        if (nodeSearch.position ||
            nodeSearch.n == firstLeaf) {  //the key to delete does not appear in the none-leaf node.
            nodeSearch.n->RemoveKey(keyvalue);
            keycount--;
            DeleteAdjust(nodeSearch.n);
        } else {  // the key to delete also appears in the none-leaf node
            struct KeyInfo keySearch;
            parentNode = nodeSearch.n->parent;
            while (parentNode) {
                parentNode->FindKey(keyvalue, keySearch);
                if (keySearch.ifExist) {
                    parentNode->keys[keySearch.position] = nodeSearch.n->keys[1];
                    break;
                }
                parentNode = parentNode->parent;
            }
            nodeSearch.n->RemoveKey(keyvalue);
            keycount--;
            DeleteAdjust(nodeSearch.n);
        }
    }
}

//Get the position of one child node in its parent node.
template<class Type>
int BPlusTree<Type>::GetChildPos(BNode *parent, BNode *child) {
    int pos = -1, i;
    for (i = 0; i <= parent->keycount; i++) {
        if (parent->childs[i] == child) {
            pos = i;
            break;
        }
    }
    return pos;
}

//adjust the B+ tree after deletion.
template<class Type>
void BPlusTree<Type>::DeleteAdjust(BNode *curNode) {
    int minNum = (degree - 1) / 2, childPos, i;
    BNode *parentNode, *siblingNode;
    while (1) {
        if (curNode->parent == NULL) {
            if (curNode->keycount == 0) {  //the root become null after deletion.
                if (curNode->leaf) {
                    delete curNode;
                    level--;
                    nodecount = keycount = 0;
                    root = NULL;
                    firstLeaf = NULL;
                } else {
                    root = curNode->childs[0];
                    delete curNode;
                    root->parent = NULL;
                    level--;
                    nodecount--;
                }
            }
            return;
        }
        if (curNode->keycount < minNum) {  //the min number of keys a node should have
            parentNode = curNode->parent;
            childPos = GetChildPos(parentNode, curNode);
            //leaf node
            if (curNode->leaf) {
                //the node has its right sibling and it has enough keys
                if (childPos < parentNode->keycount && parentNode->childs[childPos + 1]->keycount > minNum) {
                    siblingNode = parentNode->childs[childPos + 1];
                    curNode->keys[curNode->keycount] = siblingNode->keys[0];
                    curNode->vals[curNode->keycount] = siblingNode->vals[0];
                    curNode->keycount++;
                    siblingNode->RemoveKey(siblingNode->keys[0]);
                    parentNode->keys[childPos] = siblingNode->keys[0];
                    return;
                }
                    //the node has its left sibling and it has enough keys
                else if (childPos > 0 && parentNode->childs[childPos - 1]->keycount > minNum) {
                    siblingNode = parentNode->childs[childPos - 1];
                    for (i = curNode->keycount; i > 0; i--) {
                        curNode->keys[i] = curNode->keys[i - 1];
                        curNode->vals[i] = curNode->vals[i - 1];
                    }
                    curNode->keys[0] = siblingNode->keys[siblingNode->keycount - 1];
                    curNode->vals[0] = siblingNode->vals[siblingNode->keycount - 1];
                    curNode->keycount++;
                    siblingNode->RemoveKey(siblingNode->keys[siblingNode->keycount - 1]);
                    parentNode->keys[childPos - 1] = curNode->keys[0];
                    return;
                }
                    //the node must be merged with its left sibling or right sibling
                else {
                    if (childPos < parentNode->keycount) {  //Be merged with right sibling
                        siblingNode = parentNode->childs[childPos + 1];
                        for (i = 0; i < siblingNode->keycount; i++) {
                            curNode->keys[i + curNode->keycount] = siblingNode->keys[i];
                            curNode->vals[i + curNode->keycount] = siblingNode->vals[i];
                        }
                        parentNode->RemoveKey(parentNode->keys[childPos]);
                        curNode->keycount += siblingNode->keycount;
                        curNode->next = siblingNode->next;
                        delete siblingNode;
                        nodecount--;
                    } else {  //Be merged with left sibling
                        siblingNode = parentNode->childs[childPos - 1];
                        for (i = 0; i < curNode->keycount; i++) {
                            siblingNode->keys[i + siblingNode->keycount] = curNode->keys[i];
                            siblingNode->vals[i + siblingNode->keycount] = curNode->vals[i];
                        }
                        parentNode->RemoveKey(parentNode->keys[childPos - 1]);
                        siblingNode->keycount += curNode->keycount;
                        siblingNode->next = curNode->next;
                        delete curNode;
                        nodecount--;
                    }
                    curNode = parentNode;
                    continue;
                }
            }
                //none-leaf node
            else {
                //the node has its right sibling and it has enough keys
                if (childPos < parentNode->keycount && parentNode->childs[childPos + 1]->keycount > minNum) {
                    siblingNode = parentNode->childs[childPos + 1];
                    curNode->keys[curNode->keycount] = parentNode->keys[childPos];
                    curNode->childs[curNode->keycount + 1] = siblingNode->childs[0];
                    curNode->childs[curNode->keycount + 1]->parent = curNode;
                    curNode->keycount++;
                    parentNode->keys[childPos] = siblingNode->keys[0];
                    siblingNode->childs[0] = siblingNode->childs[1];
                    siblingNode->RemoveKey(siblingNode->keys[0]);
                    return;
                }
                    //the node has its left sibling and it has enough keys
                else if (childPos > 0 && parentNode->childs[childPos - 1]->keycount > minNum) {
                    siblingNode = parentNode->childs[childPos - 1];
                    for (i = curNode->keycount; i > 0; i--) {
                        curNode->keys[i] = curNode->keys[i - 1];
                        curNode->childs[i + 1] = curNode->childs[i];
                    }
                    curNode->childs[1] = curNode->childs[0];
                    curNode->childs[0] = siblingNode->childs[siblingNode->keycount];
                    if (curNode->childs[0]) {
                        curNode->childs[0]->parent = curNode;
                    }
                    curNode->keys[0] = parentNode->keys[childPos - 1];
                    curNode->keycount++;
                    parentNode->keys[childPos - 1] = siblingNode->keys[siblingNode->keycount - 1];
                    siblingNode->RemoveKey(siblingNode->keys[siblingNode->keycount - 1]);
                    return;
                }
                    //the node must be merged with its left sibling or right sibling
                else {
                    if (childPos < parentNode->keycount) {  //Be merged with right sibling
                        siblingNode = parentNode->childs[childPos + 1];
                        curNode->keys[curNode->keycount] = parentNode->keys[childPos];
                        curNode->keycount++;
                        for (i = 0; i < siblingNode->keycount; i++) {
                            curNode->keys[i + curNode->keycount] = siblingNode->keys[i];
                            curNode->childs[i + curNode->keycount] = siblingNode->childs[i];
                            curNode->childs[i + curNode->keycount]->parent = curNode;
                        }
                        curNode->childs[curNode->keycount +
                                        siblingNode->keycount] = siblingNode->childs[siblingNode->keycount];
                        curNode->childs[curNode->keycount + siblingNode->keycount]->parent = curNode;
                        curNode->keycount += siblingNode->keycount;
                        parentNode->RemoveKey(parentNode->keys[childPos]);
                        nodecount--;
                        delete siblingNode;
                    } else {   //Be merged with left sibling
                        siblingNode = parentNode->childs[childPos - 1];
                        siblingNode->keys[siblingNode->keycount] = parentNode->keys[childPos - 1];
                        siblingNode->keycount++;
                        for (i = 0; i < curNode->keycount; i++) {
                            siblingNode->keys[i + siblingNode->keycount] = curNode->keys[i];
                            siblingNode->childs[i + siblingNode->keycount] = curNode->childs[i];
                            siblingNode->childs[i + siblingNode->keycount]->parent = siblingNode;
                        }
                        siblingNode->childs[curNode->keycount +
                                            siblingNode->keycount] = curNode->childs[curNode->keycount];
                        siblingNode->childs[curNode->keycount + siblingNode->keycount]->parent = curNode;
                        siblingNode->keycount += curNode->keycount;
                        parentNode->RemoveKey(parentNode->keys[childPos - 1]);
                        nodecount--;
                        delete curNode;
                    }
                    curNode = parentNode;
                    continue;
                }
            }
        } else
            break;
    }
}

//Read the key value from the Disk.
template<class Type>
void BPlusTree<Type>::ReadFromDisk(blockNode *btmp) {
    int valsize = sizeof(int);
    char *_index = bufferMa.GetContent(*btmp);
    char *_value = _index + keysize;
    Type key;
    int val;
    while (_value - bufferMa.GetContent(*btmp) < bufferMa.GetUsingSize(*btmp)) {
        key = *(Type *) _index;
        val = *(int *) _value;
        InsertKey(key, val);
        _value += keysize + valsize;
        _index += keysize + valsize;
    }
}

//Read all keys and values from the disk.
template<class Type>
void BPlusTree<Type>::ReadFromDiskAll() {
    file = bufferMa.GetFile(fileName.c_str());
    blockNode *btmp = bufferMa.GetBlockHead(file);
    while (1) {
        if (!btmp) return;
        ReadFromDisk(btmp);
        if (btmp->ifbottom) break;
        btmp = bufferMa.GetNextBlock(file, btmp);
    }
}

//Write back
template<class Type>
void BPlusTree<Type>::WriteBackToDiskAll() {
    blockNode *btmp = bufferMa.GetBlockHead(file);
    BNode *ntmp = firstLeaf;
    int valsize = sizeof(int), i;
    while (ntmp) {
        bufferMa.SetUsingSize(*btmp, 0);
        bufferMa.SetDirty(*btmp);
        for (i = 0; i < ntmp->keycount; i++) {
            char *key = (char *) &(ntmp->keys[i]);
            char *val = (char *) &(ntmp->vals[i]);
            memcpy(bufferMa.GetContent(*btmp) + bufferMa.GetUsingSize(*btmp), key, keysize);
            bufferMa.SetUsingSize(*btmp, bufferMa.GetUsingSize(*btmp) + keysize);
            memcpy(bufferMa.GetContent(*btmp) + bufferMa.GetUsingSize(*btmp), val, valsize);
            bufferMa.SetUsingSize(*btmp, bufferMa.GetUsingSize(*btmp) + valsize);
        }
        btmp = bufferMa.GetNextBlock(file, btmp);
        ntmp = ntmp->next;
    }
    while (1) {
        if (btmp->ifbottom) break;
        bufferMa.SetUsingSize(*btmp, 0);
        bufferMa.SetDirty(*btmp);
        btmp = bufferMa.GetNextBlock(file, btmp);
    }
}

//Print the information of B+ tree.
template<class Type>
void BPlusTree<Type>::PrintTree() {
    cout << "#######################DEBUG BTree Print#######################" << endl;
    cout << "name: " << fileName << ", root:" << (void *) root << ", LeafHead: " << (void *) firstLeaf;
    cout << ", KeyCount: " << keycount << ", NodeCount: " << nodecount;
    cout << ", level: " << level << endl;
    if (root)
        PrintTreeNode(root);
    cout << "###############################################################" << endl;
}

//Print all nodes in the B+ tree.
template<class Type>
void BPlusTree<Type>::PrintTreeNode(BNode *curNode) {
    int i;
    curNode->PrintNode();
    if (!curNode->leaf) {
        for (i = 0; i <= curNode->keycount; i++)
            PrintTreeNode(curNode->childs[i]);
    }
}

#endif
