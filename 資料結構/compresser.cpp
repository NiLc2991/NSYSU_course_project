// Author: 林之謙, B113040002
// Date: Dec, 09 2023
// Purpose: Implementation of a compression software with Huffman algorithm
#include<bits/stdc++.h>
#define ll unsigned long long int
using namespace std;

string HuffmanValue[256] = {""};// String array that store the Huuffman code of ASCII characters

class Node// Node of Huffman tree
{
    public:
    Node() : left( NULL ), right( NULL ), d('\0'), freq( 0 ){};
    Node( char c,int n ) : left( NULL ), right( NULL), d( c ), freq( n ){};
    Node( char c, int n, Node *l, Node *r ) : d( c ), left( l ), right( r ), freq( n ){};

    Node *left;
    Node *right;
    char d;// ASCII character
    int freq;// Frequency of the character
};

// Class for priority queue to sort
// If two character shares same freq, compare dictionary priority of characters
class cmp
{ 
public: 
    int operator() (const Node *n1, const Node *n2) 
    { 
        if(n1->freq == n2->freq) return n1->d > n2->d;
        else return n1->freq > n2->freq; 
    } 
};

// Function which generate Huffman tree
// Use priority queue to do minheap
Node* build_huff(int a[], int size)
{
    priority_queue <Node *, vector<Node *>, cmp> pq;
    for(int i = 0; i < size ; i++)
    {
        // Push every Node in to the queue
        if(a[i] != 0) pq.push(new Node(i, a[i]));
    }

    //If there are more than one Node
    // Combine the smaller two nodes
    while( pq.size() > 1)
    {
        Node *node1 = pq.top(); pq.pop();
        Node *node2 = pq.top(); pq.pop();
        //The one with bigger dictionary priority be right node
        if( node1->d > node2->d) swap(node1, node2);
        // The perent node take the smaller character to compare
        pq.push(new Node( node1->d, node1->freq + node2->freq, node1, node2 ));
    }

    Node *temp = pq.top(); pq.pop();
    return temp;
}

//Travel the Huffman tree to find Huffman code of every character
ll convert_to_huff( Node *root , string& value)
{
    ll temp = 0;  // Size of the whole compressed file
    if (root) {
        // Travel left
        value.push_back('0');
        temp = convert_to_huff(root->left, value);
        value.pop_back();

        // If leafnode( ASCII character) is found
        if (root->left == NULL && root->right == NULL ) {
            HuffmanValue[(unsigned char)root->d] = value;// Store the Huffman code of this character
            temp += value.size() * root->freq;// Memorize the size
        }

        // Travel right
        value.push_back('1');
        temp = convert_to_huff(root->right, value);
        value.pop_back();
    }
    return temp; 
}


void compressed( string orifilename, string compressedFileName )
{
    int freqList[256] = {0};// Array tp store the frequency of each character
    double oriFileSize = 0;// Uncompressed file size
    ifstream in; in.open(orifilename, ios::in | ios::binary);
    if( in.fail() ){ cout << "Failed to open uncompressed file\n"; exit(1);}
    else cout << "Successed to open uncompressed file\n";

    /////read in binary file to string
    stringstream ss;
	ss << in.rdbuf();
	string finputString;
	finputString = ss.str();

    //////compute oriFileSize
    in.seekg(0, ios::end);
    oriFileSize = in.tellg() ;

    in.close();

    /////counting frequency
    for(int i=0; i < finputString.size(); i++)
    {
        freqList[(unsigned char)finputString[i]]++;
    }

    //////build huffman tree
    Node * root = build_huff(freqList, 256);

    //////store huffman code in HuffmanValue
    string huffValue = "";
    ll ComFileSize = convert_to_huff(root, huffValue);

/////generate compressed file
    fstream out; out.open(compressedFileName, ios::out | ios::binary);
    double headerSize = 0;// Size of file header
    string FileHeader;// Content of the compressed file

    //// Write huffman code table to file
    FileHeader += "Huffman code table:\n"; 
    for(int i = 0 ; i < 256 ; i++)
    {
        if( freqList[i] != 0)
        {
            FileHeader.push_back(i);
            FileHeader += "=";
            FileHeader += HuffmanValue[i];
            FileHeader.push_back('\n');
            headerSize += (sizeof("1=\n") - 1) ;
            headerSize += HuffmanValue[i].size() ;
        }
    }

    /// Write compressed file code to file
    double comprssedHuffCodeSize = 0; //Size of the compressed file-- only size of Huffman table in this section
    string comprssedFileHuffCode;//File contents been compressed
    comprssedFileHuffCode += "Compressed file:\n";
    for(int i = 0 ; i < finputString.size() ; i++)
    {
            comprssedFileHuffCode += HuffmanValue[(unsigned char)finputString[i]];
            comprssedHuffCodeSize += HuffmanValue[(unsigned char)finputString[i]].size();
    }comprssedHuffCodeSize /= 8.0;

    ///store info of size
    string sizeInfo = "";// Size of the information of size and ratio
    comprssedHuffCodeSize += headerSize;// Add the file header size with size of compressed file
    sizeInfo += "Uncompressed file size: "; sizeInfo += to_string(oriFileSize); sizeInfo += " bytes\n";
    sizeInfo += "Compressed file size: "; sizeInfo += to_string(comprssedHuffCodeSize); sizeInfo += " bytes\n";
    sizeInfo += "Compress ratio: "; sizeInfo += to_string(oriFileSize / comprssedHuffCodeSize); sizeInfo += "\n";

    //Put in informations of size, File header and compressed contents
    cout << sizeInfo << FileHeader ;
    out << sizeInfo << FileHeader << comprssedFileHuffCode;
    out.close();
    cout << "Compressed file generated\n";

}

// Use unordered map to store Huffman table
void decompress( string orifilename, string UncompressedFileName)
{
    ifstream in; in.open(orifilename, ios::binary);
    if( in.fail() ){ cout << "Failed to open compressed file\n"; exit(1);}
    else cout << "Successed to open compressed file\n";
    unordered_map<string, string> table;

    string readingLine; // Line been reading
    string comCode; // The compressed code
    bool StartReadingTable = false;
    bool StartReadingComCode = false;
    while(getline(in, readingLine))
    {
        if(readingLine == "Huffman code table:")
        {
            StartReadingTable = true;
            continue;
        }
        if(readingLine == "Compressed file:")
        {
            StartReadingComCode = true;
            continue;
        }
        if(StartReadingTable && StartReadingComCode == false)
        {
            // Exception while meeting '\n' character
            if(readingLine == "")
            {
                getline(in, readingLine);
                readingLine = '\n' + readingLine;
            }

            string huffmanCode, chara;
            istringstream iss(readingLine);
            getline(iss, chara, '=');

            if(chara == "")/// Exception while meeting '='
            {
                string temp;
                chara = '=';
                getline(iss, temp, '=');
            }
            getline(iss, huffmanCode);
            table.insert({huffmanCode, chara});

        }
        // Read in compressed code
        if(StartReadingComCode == true)
        {
            comCode = readingLine;
        }

    }in.close();

    // String to store the uncompressed code and string to store reading compressed code
    string reverseCode = "", tempCode ="";
    for(unsigned int i = 0; i < comCode.size(); i++)
    {
        //Add one 0/1 character once
        tempCode.push_back(comCode[i]);
        //Check if compressed code been read is in the table
        //If found, add the origninal form of the character to reverseCode
        if(table.find(tempCode) != table.end())
        {
            reverseCode += (table.find(tempCode))->second;
            tempCode = "";
        }

    }

    //Write in Uncompressed file
    ofstream out; out.open(UncompressedFileName, ios::out | ios::binary);
    out<< reverseCode;
    cout<< "Compressed file has been uncompressed\n";
    out.close();

    return ;
}

int main(int argc, char **argv)
{
    string inputFileName;
    string outputFileName;

    //Check whether the command is right
    if(argc != 6 || ( (string)argv[1] != "-c" && (string)argv[1] != "-u")|| ( (string)argv[2] != "-i" || (string)argv[4] != "-o") )
    {
        cout<<"Please input command in form: ./hw8-b113040002 -c/-u -i infile -o outfile\n";
        exit(1);
    }
    if( (string)argv[1] == "-c" ) // compress the file
    {
        if( (string)argv[2] == "-i")
        {
            inputFileName = argv[3];
            outputFileName = argv[5];
        }
        else if( (string)argv[2] == "-o" )
        {
            outputFileName = argv[3];
            inputFileName = argv[5];
        }
        compressed(inputFileName, outputFileName);

    }
    else if( (string)argv[1] == "-u") // uncompress the file
    {
        if( (string)argv[2] == "-i")
        {
            inputFileName = argv[3];
            outputFileName = argv[5];
        }
        else if( (string)argv[2] == "-o")
        {
            outputFileName = argv[3];
            inputFileName = argv[5];
        }
        decompress(inputFileName, outputFileName);
    }
    
    
    return 0;
}
