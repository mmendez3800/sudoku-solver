/******************************************************************************
File Name:    main_algorithm_x.cpp
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
#include <map>
#include <climits>
#include <set>

using namespace std;

//Global variables that define the values of our Sudoku grid, as well as values that relate to our grid
const int SUDOKU_SIZE = 25;
const int SUDOKU_SIZE_SQUARED = SUDOKU_SIZE * SUDOKU_SIZE;
const int SUDOKU_SIZE_SQUARE_ROOT = sqrt(SUDOKU_SIZE);

//Global variables that define the values and structure of our exact cover problem
const int ROWS = SUDOKU_SIZE * SUDOKU_SIZE * SUDOKU_SIZE;
const int COLUMNS = 4 * SUDOKU_SIZE * SUDOKU_SIZE;

//Global variables to hold our provided Sudoku grid and the solution to the grid (if one exists)
int sudokuInput[SUDOKU_SIZE][SUDOKU_SIZE];
int sudokuSolution[SUDOKU_SIZE_SQUARED];

//helper struct for the original Algorithm X implementation
struct sequenceAlgorithmX {
    vector<bool> sequence;
    int numberInOriginalSequence;

    sequenceAlgorithmX() {
        numberInOriginalSequence = 0;
    }
};

//for removing/restoring ROWS only
struct removeOrRestoreHelperDSAlgorithmX {
    sequenceAlgorithmX sequence;
    int numberInOriginalSequence;

    removeOrRestoreHelperDSAlgorithmX() {
        numberInOriginalSequence = 0;
    }
};

vector<sequenceAlgorithmX> exactCoverProblem;
vector<int> currSolution;
//--------------------------------------------------------------------

//Function to initialize our exact cover problem based on the sudoku size
void initializeMatrix(vector<sequenceAlgorithmX>& matrix) {
    int Candidate, Row, Column, Box, Aux, Aux1;
    matrix.resize(ROWS);
    for(int i=0; i < ROWS; i++){
        matrix[i].numberInOriginalSequence = i;
        matrix[i].sequence.resize(COLUMNS);
        Candidate = i % SUDOKU_SIZE;
        Row = i / SUDOKU_SIZE_SQUARED;
        Aux = i / SUDOKU_SIZE;
        Column = Aux % SUDOKU_SIZE;
        Aux = Row / SUDOKU_SIZE_SQUARE_ROOT;
        Aux1 = Column / SUDOKU_SIZE_SQUARE_ROOT;
        Box = Aux * SUDOKU_SIZE_SQUARE_ROOT + Aux1;
        matrix[i].sequence[Row*SUDOKU_SIZE + Column] = true;
        matrix[i].sequence[SUDOKU_SIZE_SQUARED + Row * SUDOKU_SIZE + Candidate] = true;
        matrix[i].sequence[2*SUDOKU_SIZE_SQUARED + Column * SUDOKU_SIZE + Candidate] = true;
        matrix[i].sequence[3*SUDOKU_SIZE_SQUARED + Box * SUDOKU_SIZE + Candidate] = true;
    }
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
Function:     readInputGrid
Description:  This function reads the text file which is to contained a
              predefined Sudoku grid. The contents of the grid are then stored
              into a two-dimensional array which will be referenced throughout
              the Sudoku solver program.
Input:        fileName  - The text file containing the Sudoku grid
Result:       Reads the text file containing a predefined Sudoku grid and
              stores the contents into a global two-dimensional array
******************************************************************************/
int readInputGrid(const char* filename) {
    //Reads the specified file and checks if the file is not open
    ifstream infile(filename);
    if (!infile.is_open()) {
        return -1;
    }

    //Places the contents of the file into our global two-dimensional array
    for(int i = 0 ; i < SUDOKU_SIZE ; ++i) {
        for(int j = 0 ; j < SUDOKU_SIZE ; ++j) {
            infile >> sudokuInput[i][j];
        }
    }

    infile.close();
    return 0;
}

/******************************************************************************
Function:     removeRowsAndColumnsForGivenRow
Description:  This function, given a row number, deletes the columns containing
              1s and for each of these columns, it proceeds to delete their rows
              with 1s in the given column. It is equivalent to cover method
              in Dancing Links version of the algorithm
Input:        row  - number of row to be deleted
              deletedColumns - vector that will contain deleted columns
              deletedRows - vector that will contain deleted rows (the auxiliary
              struct type removeOrRestoreHelperDS is needed to also retain the
              original row numbers)
Result:       Removal of columns and rows in order to satisfy the constraints
              of the Sudoku problem
******************************************************************************/
void removeRowsAndColumnsForGivenRow(int row,
                                     vector<sequenceAlgorithmX>& deletedColumns,
                                     vector<removeOrRestoreHelperDSAlgorithmX>& deletedRows) {
    vector<int> columnsToDelete;
    set<int> rowsToDelete;

    //Selects columns to be deleted
    for(int i = 0 ; i < exactCoverProblem[row].sequence.size() ; ++i) {
        if(exactCoverProblem[row].sequence[i]) {
            columnsToDelete.push_back(i);
        }
    }

    deletedColumns.resize(columnsToDelete.size());

    //Deletes columns first
    for(int i = 0 ; i < columnsToDelete.size() ; ++i) {
        sequenceAlgorithmX deletedColumn;
        deletedColumn.numberInOriginalSequence = columnsToDelete[i];
        //With each deleted column, we have to decrement the column numbers, so as not to refer to non-existing columns
        columnsToDelete[i] -= i;
        deletedColumn.sequence.resize(exactCoverProblem.size());

        //In the meantime, we select rows to be deleted
        for(int j = 0 ; j < exactCoverProblem.size() ; ++j) {
            if(exactCoverProblem[j].sequence[columnsToDelete[i]]) {
                rowsToDelete.insert(j);
            }

            //Column deletion
            deletedColumn.sequence[j] = exactCoverProblem[j].sequence[columnsToDelete[i]];
            exactCoverProblem[j].sequence.erase(exactCoverProblem[j].sequence.begin()+columnsToDelete[i]);
        }

        deletedColumns[i] = deletedColumn;
    }

    deletedRows.resize(rowsToDelete.size());

    //Delets rows
    int deletedSoFar = 0;
    for(set<int>::iterator it = rowsToDelete.begin() ; it != rowsToDelete.end() ; ++it, ++deletedSoFar) {
        removeOrRestoreHelperDSAlgorithmX deletedRow;
        deletedRow.numberInOriginalSequence = *it;
        //Like with columns, with each deleted row we have to decrement the consecutive numbers of rows to be deleted
        int rowToDelete = *it - deletedSoFar;
        deletedRow.sequence = exactCoverProblem[rowToDelete];
        deletedRows[deletedSoFar] = deletedRow;

        exactCoverProblem.erase(exactCoverProblem.begin()+rowToDelete);
    }
}

/******************************************************************************
Function:     restoreRowsAndColumnsForGivenRow
Description:  This function restores columns and rows when the algorithm backtracks
              columnsToRestore - vector containing columns to be restored
              rowsToRestore - vector containing rows to be restored
Result:       Given columns and rows have been restored to their original
              positions within the binary matrix
******************************************************************************/
void restoreRowsAndColumnsForGivenRowAlgorithmX(const vector<sequenceAlgorithmX>& columnsToRestore,
                                                const vector<removeOrRestoreHelperDSAlgorithmX>& rowsToRestore) {
    //First restore rows
    for(int i = 0 ; i < rowsToRestore.size() ; ++i) {
        exactCoverProblem.insert(exactCoverProblem.begin()+rowsToRestore[i].numberInOriginalSequence, rowsToRestore[i].sequence);
    }

    //Then restore columns
    for(int i = 0 ; i < columnsToRestore.size() ; ++i) {
        int currColBeingRestored = columnsToRestore[i].numberInOriginalSequence;
        for(int j = 0 ; j < exactCoverProblem.size() ; ++j) {
            exactCoverProblem[j].sequence.insert(exactCoverProblem[j].sequence.begin()+currColBeingRestored, columnsToRestore[i].sequence[j]);
        }
    }
}

/******************************************************************************
Function:     convertSudokuGridToAlgorithmX
Description:  This function converts the initially filled sudoku grid to its
              binary matrix representation. For each pre-filled cell, we "cover"
              the corresponding row in the binary matrix.
Result:       Our binary matrix is adjusted to a specific Sudoku problem and can
              be processed by the recursive search() function at this time
******************************************************************************/
void convertSudokuGridToAlgorithmX(int input[SUDOKU_SIZE][SUDOKU_SIZE]) {
    for(int inputR = 0 ; inputR < SUDOKU_SIZE ; ++inputR) {
        for (int inputC = 0; inputC < SUDOKU_SIZE; ++inputC) {
            if (input[inputR][inputC] != -1) {
                bool ifContinue = true;
                for (int i = 0 ; i < exactCoverProblem.size() && ifContinue ; i++) {
                    int realRowNumber = exactCoverProblem[i].numberInOriginalSequence;
                    int Row, Column, Candidate, Aux;
                    Candidate = realRowNumber % SUDOKU_SIZE;
                    Row = realRowNumber / SUDOKU_SIZE_SQUARED;
                    Aux = realRowNumber / SUDOKU_SIZE;
                    Column = Aux % SUDOKU_SIZE;

                    for (int j = 0; j < exactCoverProblem[i].sequence.size() && ifContinue; j++) {
                        //Checks if the row node corresponds to the value and position of the cell within the Sudoku grid
                        if (exactCoverProblem[i].sequence[j] && (Candidate == input[inputR][inputC]) &&
                            Row == inputR && Column == inputC) {

                            //Deletes (or "covers" in Dancing Links terminology) the corresponding columns and rows
                            //that represent our initially given Sudoku grid values
                            vector<sequenceAlgorithmX> deletedColumnsDUMMY; //dummy, because we're not gonna need them here
                            vector<removeOrRestoreHelperDSAlgorithmX> deletedRowsDUMMY;
                            removeRowsAndColumnsForGivenRow(i, deletedColumnsDUMMY, deletedRowsDUMMY);

                            //Stores the value of the cell within our Sudoku grid
                            //Will be used for reference later if a solution can be found
                            sudokuSolution[(Row * SUDOKU_SIZE) + Column] = input[inputR][inputC];

                            ifContinue = false;
                        }
                    }
                }
            }
        }
    }
}

/******************************************************************************
Function:     chooseProperColumnAlgorithmX
Description:  This function implements the heuristic suggested by D. Knuth,
              it searches for the column in the current binary matrix representation
              with the smallest amount of 1s in it
Result:       Number of the column with the smallest amount of 1s
******************************************************************************/
int chooseProperColumnAlgorithmX() {
    int columnNumber;
    int minNumberOfOnes = INT_MAX;

    for(int col = 0, currNumberOfOnes = 0 ; col < exactCoverProblem[0].sequence.size() ; ++col, currNumberOfOnes = 0) {
        for(int row = 0 ; row < exactCoverProblem.size() ; ++row) {
            if(exactCoverProblem[row].sequence[col]) {
                ++currNumberOfOnes;
            }
        }
        if(currNumberOfOnes < minNumberOfOnes) {
            minNumberOfOnes = currNumberOfOnes;
            columnNumber = col;
        }
    }

    return columnNumber;
}

/******************************************************************************
Function:     search
Description:  Recursive backtracking function which aims to search for a solution
              to the given Sudoku problem instance. It mimics the behavior of
              search for Dancing Links version, i.e. we keep deleting rows and
              columns, backtracking when necessary, until our exact cover binary
              matrix is empty, which amounts to a solution to our Sudoku grid.
Result:       Sudoku problem instance solved, with SUDOKU_SOLUTION array filled
              with all of the Sudoku cell values. The boolean return value of "true"
              means "proceed with recursive calls, because no solution has been
              found at this point". Therefore, the boolean value of "false" denotes
              a success in finding a solution.
******************************************************************************/
bool search() {
    //if the matrix has no COLUMNS, then we found a solution
    if (exactCoverProblem.empty()) {
        for(int i = 0 ; i < currSolution.size() ; ++i) {
            int actualRow = currSolution[i];
            int Row, Column, Candidate, Aux;
            Candidate = actualRow % SUDOKU_SIZE;
            Row = actualRow / SUDOKU_SIZE_SQUARED;
            Aux = actualRow / SUDOKU_SIZE;
            Column = Aux % SUDOKU_SIZE;

            sudokuSolution[(Row * SUDOKU_SIZE) + Column] = Candidate;
        }
        printSolution(sudokuSolution);
        return false;
    }

    //At first, locates the column that has the smallest number of row nodes
    //(used as a heuristic when performing our search)
    for(int row = 0, columnToProcess = chooseProperColumnAlgorithmX() ; row < exactCoverProblem.size() ; ++row) {
        //Covers our column with the smallest size
        if(exactCoverProblem[row].sequence[columnToProcess]) {
            currSolution.push_back(exactCoverProblem[row].numberInOriginalSequence);

            vector<sequenceAlgorithmX> deletedColumns;
            vector<removeOrRestoreHelperDSAlgorithmX> deletedRows;
            removeRowsAndColumnsForGivenRow(row, deletedColumns, deletedRows);

            //Recursively calls the search function and checks if the value returned is false
            if(!search()) {
                return false;
            }

            restoreRowsAndColumnsForGivenRowAlgorithmX(deletedColumns, deletedRows);

            //Backtracking
            currSolution.pop_back();
        }
    }
    return true;
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
int sudokuSolver(const char* filename) {
    //Checks if the text file could be read by the solver
    if(readInputGrid(filename) != 0) {
        cerr << "File could not be read";
        return -1;
    }

    //Creates, builds, and updates our doubly-linked list data structure
    initializeMatrix(exactCoverProblem);
    convertSudokuGridToAlgorithmX(sudokuInput);
    if(search()) {
        cout << "No Solution Found\nFail";
    }

    return 0;
}

/******************************************************************************
Function:     main
Description:  This function executes the Sudoku solver program using the
              indicated text file specified on the command line as the first
              argument. As the solver is being run, it tracks the
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
    ios::sync_with_stdio(false);

    //Executes the Sudoku solver while tracking the time in which the function started/ended
    auto t1 = std::chrono::high_resolution_clock::now();
    int returnCode = sudokuSolver(argv[1]);
    auto t2 = std::chrono::high_resolution_clock::now();

    //Calculates the time (in milliseconds) in which the Sudoku solver completed
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    std::cout << "\nSudoku Solver took " << duration << " ms to complete.\n";
    return returnCode;
}
