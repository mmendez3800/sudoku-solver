/******************************************************************************
File Name:    main_dancing_links.cpp
Description:  This program solves a given Sudoku grid of size 25 by 25. The
              programs looks for a particular text file which contains a
              predetermined Sudoku grid. The grid is filled with values ranging
              from -1 to 24. The value -1 indicates a space in the grid while
              the values 0 to 24 represent initial values. The program will
              then either provide a solution to the Sudoku grid or indicate
              that no solution is possible.
******************************************************************************/


#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <chrono>
#include <string>

using namespace std;
    

/******************************************************************************
Structure:    Node
Description:  This structure is a Node that represents a cell or column within
              the doubly-linked list data structure. The Node contains pointers
              to its neighbors as well as to the column the Node is in (if not
              a column Node). The Node also retains the size of the column (if
              a column Node) as well as provides an array that allows us to
              identify which cell within the Sudoku grid it represents.
Data Member:  leftNode  - Pointer to Node that is left of current Node
              rightNode  - Pointer to Node that is right of current Node
              upperNode  - Pointer to Node that is above current Node
              lowerNode  - Pointer to Node that is below current Node
              columnNode  - Pointer to column Node the current Node (indicates
                            which column the Node is in)
              columnSize  - The size of the column
              identifier  - Manner in which to indify the non-column Node
******************************************************************************/
struct Node {
    
    //Links the node to its surrounding nodes
    Node* leftNode;
    Node* rightNode;
    Node* upperNode;
    Node* lowerNode;

    //If NOT a column node, points to which column the node is in
    Node* columnNode;

    //The size of the column (to be used by column node)
    int columnSize;

    /*
    Array used to identify the the value, row, and column associated to a non-column node
    This is in relation to the node's position within the exact cover problem
    For example, if the value of a node's identifier is [5, 9, 15], it means the following:
      - The value assigned to the node is 5 within the sudoku grid
      - The node is located within row 9 of the sudoku grid
      - The node is located within column 15 of the sudoku grid
    */
    int identifier[3];
};


//Global variables that define the values of our Sudoku grid, as well as values that relate to our grid
const int SUDOKU_SIZE = 25;
const int SUDOKU_SIZE_SQUARED = SUDOKU_SIZE * SUDOKU_SIZE;
const int SUDOKU_SIZE_SQUARE_ROOT = sqrt(SUDOKU_SIZE);

//Global variables that define the values and structure of our exact cover problem
const int ROWS = SUDOKU_SIZE * SUDOKU_SIZE * SUDOKU_SIZE;
const int COLUMNS = 4 * SUDOKU_SIZE * SUDOKU_SIZE;
bool EXACT_COVER_PROBLEM[ROWS][COLUMNS] = { { 0 } };

//Global variables to hold our provided Sudoku grid and the solution to the grid (if one exists)
int SUDOKU_INPUT[SUDOKU_SIZE][SUDOKU_SIZE];
int SUDOKU_SOLUTION[SUDOKU_SIZE_SQUARED] = { -1 };

//Global vector to retain the list of Nodes used within our search for a solution
vector<Node*> CURRENT_SOLUTION;

//Global Node pointer that is to represent our head node within the doubly-linked list structure
Node* headNode;


void coverColumn(Node* column);
void printSolution(int solution[SUDOKU_SIZE_SQUARED]);


/******************************************************************************
Function:     readInputGrid
Description:  This function reads the text file which is to contained a
              predefined Sudoku grid. The contents of the grid are then stored
              into a two-dimensional array which will be referenced throughout
              the Sudoku solver program.
Input:        fileName  - The text file containing the Sudoku grid
Result:       Reads the text file containing a predefined Sudoku grid and
              stores the contents into a global two-dimensional array
******************************************************************************/
int readInputGrid(const char* fileName) {

    //Reads the specified file and checks if the file is not open
    ifstream inFile(fileName);
    if (!inFile.is_open()) {
        return -1;
    }

    //Places the contents of the file into our global two-dimensional array
    for(int i = 0 ; i < SUDOKU_SIZE ; ++i) {
        for(int j = 0 ; j < SUDOKU_SIZE ; ++j) {
            inFile >> SUDOKU_INPUT[i][j];
        }
    }

    inFile.close();
    return 0;
}


/******************************************************************************
Function:     initializeExactCoverProblem
Description:  This function initializes our exact cover problem. The
              constraints in regard having a value in every cell, as well as
              the constraints of having a unique value in every row, column,
              and box, are determined based on the size of the Sudoku grid.
              Once the calculation is complete, it places a value of 1
              within each appropriate element of the two-dimensional array to
              represent the determined constraintconstraint.
Input:        ECP  - The exact cover problem
Result:       Calculates the constraints of the exact cover problem based on
              the size of the Sudoku grid
******************************************************************************/
void initializeExactCoverProblem(bool ECP[ROWS][COLUMNS]) {
    int candidate, row, column, box;
    for (int i = 0; i < ROWS; i++) {

        //Perform calculations to deterine exact cover problem constraints
        candidate = i % SUDOKU_SIZE;
        row = (int)(i / SUDOKU_SIZE_SQUARED);
        column = (int)(i / SUDOKU_SIZE) % SUDOKU_SIZE;
        box = (int)(row / SUDOKU_SIZE_SQUARE_ROOT) * SUDOKU_SIZE_SQUARE_ROOT + (int)(column / SUDOKU_SIZE_SQUARE_ROOT);

        //Initialize the first constraint of having one value in every cell
        ECP[i][0 * SUDOKU_SIZE_SQUARED + row * SUDOKU_SIZE + column] = 1;

        //Initialize the second constraint of having one unique value in every row
        ECP[i][1 * SUDOKU_SIZE_SQUARED + row * SUDOKU_SIZE + candidate] = 1;

        //Initialize the third constraint of having one unique value in every column
        ECP[i][2 * SUDOKU_SIZE_SQUARED + column * SUDOKU_SIZE + candidate] = 1;

        //Inialize the fourth constraint of having one unique value in every square
        ECP[i][3 * SUDOKU_SIZE_SQUARED + box * SUDOKU_SIZE + candidate] = 1;   
    }
}


/******************************************************************************
Function:     initializeDoublyLinkedList
Description:  This function converts our exact cover problem into a
              doubly-linked list data structure. The structure consists of a
              head node. From there, column nodes are created, linked to each
              other and the head node. The column nodes will represent each
              column in the exact cover problem. Once completed, row nodes are
              created and linked to each other (as well as their corresponding
              column node). The row nodes will represent each value of 1 found
              within the exact cover problem.
Input:        ECP  - The exact cover problem
Result:       Converts the exact cover problem into a doubly-linked list data
              structure
******************************************************************************/
void initializeDoublyLinkedList(bool ECP[ROWS][COLUMNS]) {

    headNode = new Node;

    //Creates our column nodes and links them to each other, as well as to our head node
    Node* columnNode = headNode;
    for (int i = 0; i < COLUMNS; i++) {
        Node* newNode = new Node;
        newNode->leftNode = columnNode;
        newNode->rightNode = headNode;
        newNode->upperNode = newNode;
        newNode->lowerNode = newNode;

        newNode->columnSize = 0;

        columnNode->rightNode = newNode;
        columnNode = newNode;
        headNode->leftNode = columnNode;
    }

    //Loops through each row within the exact cover problem
    for (int i = 0; i < ROWS; i++) {

        //Perform calculations based on each row being looped through
        //Will be used when determining the indentifier of each row node created
        int candidate, row, column;
        candidate = i % SUDOKU_SIZE;
        row = (int)(i / SUDOKU_SIZE_SQUARED);
        column = (int)(i / SUDOKU_SIZE) % SUDOKU_SIZE;

        columnNode = headNode->rightNode;
        Node* previousNode = nullptr;

        //Loops through each column within the ROWS of the exact cover problem
        for (int j = 0; j < COLUMNS; j++, columnNode = columnNode->rightNode) {

            //Creates our node if a value of '1' is present within the exact cover problem
            if (ECP[i][j]) {
                Node* newNode = new Node;

                //Assigns initial indentifier values to the newly created node and updates column information
                newNode->identifier[0] = candidate;
                newNode->identifier[1] = row;
                newNode->identifier[2] = column;
                newNode->columnNode = columnNode;
                columnNode->columnSize++;

                //Assigns the left and right links of the node
                //Assigns the left and right links of the previous node, as well as the first node in the row
                if (previousNode == nullptr) {
                    previousNode = newNode;
                    previousNode->rightNode = newNode;
                    previousNode->leftNode = newNode;
                }
                else {
                    newNode->leftNode = previousNode;
                    newNode->rightNode = previousNode->rightNode;
                    newNode->rightNode->leftNode = newNode;
                    previousNode->rightNode = newNode;
                }

                //Assigns the upper and lower links of the node
                //Assigns the upper and lower links of the column node
                newNode->upperNode = columnNode->upperNode;
                newNode->lowerNode = columnNode;
                columnNode->upperNode->lowerNode = newNode;
                columnNode->upperNode = newNode;
                previousNode = newNode;
            }
        }
    }
}


/******************************************************************************
Function:     updateDoublyLinkedList
Description:  This function updates the doubly-linked list data structure to
              recognize the Sudoku puzzle given to the solver. For every value
              given within the input, the structure is updated to indicate that
              certain Sudoku constraints have been met.
Input:        input  - The initial Sudoku grid provided to the solver
Result:       Updates the doubly-linked list data structure to reflect the
              initial values given to the solver
******************************************************************************/
void updateDoublyLinkedList(int input[SUDOKU_SIZE][SUDOKU_SIZE]) {

    //Loops through every column and row within our doubly-linked list
    for (Node* columnNode = headNode->rightNode; columnNode != headNode; columnNode = columnNode->rightNode) {
        for (Node* rowNode = columnNode->lowerNode; rowNode != columnNode; rowNode = rowNode->lowerNode) {

            //Checks if the row node corresponds to the value and position of the cell within the Sudoku grid
            if (rowNode->identifier[0] == input[rowNode->identifier[1]][rowNode->identifier[2]]) {

                //Stores the value of the cell within our Sudoku grid
                //Will be used for reference later if a solution can be found
                SUDOKU_SOLUTION[(rowNode->identifier[1] * SUDOKU_SIZE) + rowNode->identifier[2]] = input[rowNode->identifier[1]][rowNode->identifier[2]];

                //Covers the corresponding columns and rows that represent our initially given Sudoku grid values
                coverColumn(columnNode);
                for (Node* rightLink = rowNode->rightNode; rightLink != rowNode; rightLink = rightLink->rightNode) {
                    coverColumn(rightLink->columnNode);
                }
            }
        }
    }
}


/******************************************************************************
Function:     coverColumn
Description:  This function updates the doubly-linked list data structure and
              "covers" all nodes within the column by reassigning the links of
              the surrounding nodes. This allows for the structure to reflect
              that a particular column does not exist.
Input:        column  - The column node used to indicate that the contents of
                        the entire column are being "covered"
Result:       Updates the doubly-linked list data structure to reflect that
              the column no longer exists
******************************************************************************/
void coverColumn(Node* column) {

    //Reassigns the links for the column node
    column->rightNode->leftNode = column->leftNode;
    column->leftNode->rightNode = column->rightNode;

    //Traverses through each row within the column, as well as through each node within that row
    for (Node* rowNode = column->lowerNode; rowNode != column; rowNode = rowNode->lowerNode) {
        for (Node* rightLink = rowNode->rightNode; rightLink != rowNode; rightLink = rightLink->rightNode) {

            //Reassigns the links for each node within the row
            rightLink->lowerNode->upperNode = rightLink->upperNode;
            rightLink->upperNode->lowerNode = rightLink->lowerNode;

            //Notes that the size of the column has decreased by "covering" the row
            rightLink->columnNode->columnSize--;
        }
    }
}


/******************************************************************************
Function:     uncoverColumn
Description:  This function updates the doubly-linked list data structure and
              "uncovers" all nodes within the column by reassigning the links
              of the surrounding nodes. This allows for the structure to
              reflect that a particular column has been added back into the
              structure.
Input:        column  - The column node used to indicate that the contents of
                        the entire column are being "uncovered"
Result:       Updates the doubly-linked list data structure to reflect that
              the column has been readded
******************************************************************************/
void uncoverColumn(Node* column) {

    //Traverses through each row within the column, as well as through each node within that row
    for (Node* rowNode = column->upperNode; rowNode != column; rowNode = rowNode->upperNode) {
        for (Node* leftLink = rowNode->leftNode; leftLink != rowNode; leftLink = leftLink->leftNode) {

            //Notes that the size of the column has increased by "uncovering" the row
            leftLink->columnNode->columnSize++;

            //Reassigns the links for each node within the row
            leftLink->upperNode->lowerNode = leftLink;
            leftLink->lowerNode->upperNode = leftLink;
        }
    }

    //Reassigns the links for the column node
    column->leftNode->rightNode = column;
    column->rightNode->leftNode = column;
}


/******************************************************************************
Function:     search
Description:  This function performs a search on the updated doubly-linked list
              data structure. THe function looks through the columns of the
              structure and "covers" the column with the smallest size. From
              there, the search is performed recursively until either a
              solution has been found or it is determined that no solution
              exists.
Result:       Returns false is the search has found a solution to the given
              Sudoku grid
              Returns true if the search has not found a solution to the given
              Sudoku grid
******************************************************************************/
bool search() {

    //Checks if the right link of the head node is itself
    //This indicates that a solution has been found for the given Sudoku grid
    if (headNode->rightNode == headNode) {
        for(int i = 0 ; i < CURRENT_SOLUTION.size() ; ++i) {
            SUDOKU_SOLUTION[(CURRENT_SOLUTION[i]->identifier[1] * SUDOKU_SIZE) + CURRENT_SOLUTION[i]->identifier[2]] = CURRENT_SOLUTION[i]->identifier[0];
        }
        printSolution(SUDOKU_SOLUTION);
        return false;
    }

    //Locates the column that has the smallest number of row nodes
    //Used as a heuristic when performing our search
    Node* chosenColumn = headNode->rightNode;
    for (Node* currentColumn = chosenColumn->rightNode; currentColumn != headNode; currentColumn = currentColumn->rightNode) {
        if (currentColumn->columnSize < chosenColumn->columnSize) {
            chosenColumn = currentColumn;
        }
    }

    //Covers our column with the smallest size
    coverColumn(chosenColumn);

    //Loops through every row within the selected column
    for (Node* rowNode = chosenColumn->lowerNode; rowNode != chosenColumn; rowNode = rowNode->lowerNode) {

        //Saves the value of the row node into our global variable
        //To be referenced for later if row node is part of our solution
        CURRENT_SOLUTION.push_back(rowNode);

        //Loops through every node within our row and covers the column the node is in
        for (Node* rightLink = rowNode->rightNode; rightLink != rowNode; rightLink = rightLink->rightNode) {
            coverColumn(rightLink->columnNode);
        }

        //Recursively calls the search function and checks if the value returned is false
        if(!search()) {
            return false;
        }

        //If current row is not a part of our solution, looks for the last row node added to solution
        //Determines the last column that was referenced with regard to the row node
        chosenColumn = CURRENT_SOLUTION.back()->columnNode;

        //Loops through every node within our row and uncovers the column the node is in
        for (Node* leftLink = CURRENT_SOLUTION.back()->leftNode; leftLink != CURRENT_SOLUTION.back(); leftLink = leftLink->leftNode) {
            uncoverColumn(leftLink->columnNode);
        }

        //Removes last row node added due to not being part of current solution
        CURRENT_SOLUTION.pop_back();
    }

    //Uncovers our column selected
    uncoverColumn(chosenColumn);

    return true;
}


/******************************************************************************
Function:     printSolution
Description:  This function prints the solution to the predefined Sudoku grid
              (if such a solution exists). As the solution is printed, there
              are separators in place to help the user read the filled Sudoku
              grid solution.
Input:        solution  - The solution to the Sudoku grid provided
Result:       Prints the predefined Sudoku grid with all empty spaces filled
              such that a solution is given
******************************************************************************/
void printSolution(int solution[SUDOKU_SIZE_SQUARED]) {
    for (int i = 0; i < SUDOKU_SIZE; i++) {
        for (int j = 0; j < SUDOKU_SIZE; j++) {
            if (j % SUDOKU_SIZE_SQUARE_ROOT == 0) {
                cout << "|  ";
            }
            if (0 <= solution[(i * SUDOKU_SIZE) + j] && solution[(i * SUDOKU_SIZE) + j] <= 9) {
                cout << " " << solution[(i * SUDOKU_SIZE) + j] << "  ";
            }
            else {
                cout << solution[(i * SUDOKU_SIZE) + j] << "  ";
            }
        }
        cout << "|" << endl;
        if ((i + 1) % SUDOKU_SIZE_SQUARE_ROOT == 0 && (i + 1) != SUDOKU_SIZE) {
            cout << string((SUDOKU_SIZE * 4) + (3 * SUDOKU_SIZE_SQUARE_ROOT) + 1, '-') << "\n";
        }
    }
}


/******************************************************************************
Function:     sudokuSolver
Description:  This function combines the previously defined functions in order
              to create a Sudoku solver. The function first reads the text file
              provided. From there, the doubly-linked data structure is created
              and updated based on the provided grid. Finally, a search is
              performed in order to provide a solution to the grid.
Input:        fileName  - A constant reference to the text file given to the
                          Sudoku solver
Result:       Returns -1 if the text file provided to the solver is not able to
              be read
              Returns 0 if the solver was able to read the file and perform a
              successful search
******************************************************************************/
int sudokuSolver(const char* fileName) {

    //Checks if the text file could be read by the solver
    if(readInputGrid(fileName) != 0) {
        cerr << "File could not be read";
        return -1;
    }

    //Creates, builds, and updates our doubly-linked list data structure
    initializeExactCoverProblem(EXACT_COVER_PROBLEM);
    initializeDoublyLinkedList(EXACT_COVER_PROBLEM);
    updateDoublyLinkedList(SUDOKU_INPUT);

    //Performs a search on the structure and checks if no solution was found
    if(search()) {
        cout << "No Solution Found\nFail";
    }

    return 0;
}


/******************************************************************************
Function:     main
Description:  This function executes the Sudoku solver program using the
              indicated text file. As the solver is being run, it tracks the
              amount of time taken to execute the solver and prints the
              resulting time.
Result:       Returns an integer stating whether the function was either able
              to exit the program successfully or not
******************************************************************************/
int main(int argc, char* argv[]) {

    //Checks if the command line arguments contain filename
    if(argc < 2) {
        cerr << "Wrong number of command-line arguments\n";
        return -1;
    }

    //Disables the synchronization between the C and C++ standard streams
    ios_base::sync_with_stdio(false);

    //Executes the Sudoku solver while tracking the time in which the function started/ended
    auto t1 = chrono::high_resolution_clock::now();
    int returnCode = sudokuSolver(argv[1]);
    auto t2 = chrono::high_resolution_clock::now();

    //Calculates the time (in milliseconds) in which the Sudoku solver completed
    auto duration = chrono::duration_cast<chrono::milliseconds>(t2 - t1).count();
    std::cout << "\nSudoku Solver took " << duration << " ms to complete\n";

    return returnCode;
}