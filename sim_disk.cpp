#include <iostream>
#include <vector>
#include <map>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

#define DISK_SIZE 256
#define occupied 1
#define freeBit 0

void decToBinary(int n , char &c) 
{ 
   // array to store binary number 
    int binaryNum[8]; 
    // counter for binary array 
    int i = 0; 
    while (n > 0) { 
          // storing remainder in binary array 
        binaryNum[i] = n % 2; 
        n = n / 2; 
        i++; 
    } 
    // printing binary array in reverse order 
    for (int j = i - 1; j >= 0; j--) {
        if (binaryNum[j]==1)
            c = c | 1u << j;
    }
 } 

// #define SYS_CALL
// ============================================================================
class fsInode {
    int fileSize;
    int block_in_use;
    int *directBlocks;
    int singleInDirect;
    int num_of_direct_blocks;
    int block_size;

    public:
    fsInode(int _block_size, int _num_of_direct_blocks) {
        fileSize = 0; 
        block_in_use = 0; 
        block_size = _block_size;
        num_of_direct_blocks = _num_of_direct_blocks;
        directBlocks = new int[num_of_direct_blocks];
		assert(directBlocks);
        for (int i = 0 ; i < num_of_direct_blocks; i++) {   
            directBlocks[i] = -1;
        }
        singleInDirect = -1;
    }

    int getBlockInUse(){
        return block_in_use;
    }
    void setBlockInUse(){ // add one block to blocks in use
        block_in_use++;
    }
    int* getDirectArr(){
        return  directBlocks;
    }
    void addToFileSize(int size){ // change file size after writing to the file
        fileSize = fileSize + size;
    }
    int getFileSize(){ 
        return fileSize;
    }
    void setSinglInDirect(int blockNum){
        singleInDirect = blockNum;
    }
    int getSinglInDirect(){
        return singleInDirect;
    }

    int nextFreeDirectBlock (){
        // returns the serial number of the free direct block in the array
        int directSize = num_of_direct_blocks * block_size;
        int i;
        if (directSize > fileSize ){
            for (i = 0 ; i < num_of_direct_blocks ; i++ ){
                if (directBlocks[i] == -1)
                    break;
            }
        if(freeBitInBlock() != 0)
            return i-1;
        return i;
        }
    }
    int freeBitInBlock (){
        //returns the number of free bit in the last block
        int f = block_in_use*block_size - fileSize;
        return f;
    }
    ~fsInode() { 
        delete directBlocks;
    }
};


// ============================================================================
class FileDescriptor {
    pair<string, fsInode*> file;
    bool inUse;

    public:
    FileDescriptor(string FileName, fsInode* fsi) {
        file.first = FileName;
        file.second = fsi;
        inUse = true;
    }

    string getFileName() {
        return file.first;
    }

    fsInode* getInode() {
        return file.second;
    }
    bool isInUse() { 
        return (inUse); 
    }
    void setInUse(bool _inUse) {
        inUse = _inUse ;
    }
     void deleteFile (){
         // delets the file's data
         file.first = "";
         inUse = false;
     }
};
 

#define DISK_SIM_FILE "DISK_SIM_FILE.txt"
// ============================================================================
class fsDisk {
    FILE *sim_disk_fd;
    bool is_formated;
	// BitVector - "bit" (int) vector, indicate which block in the disk is free
	//              or not.  (i.e. if BitVector[0] == 1 , means that the 
	//             first block is occupied. 
    int BitVectorSize;
    int *BitVector;
    // Unix directories are lists of association structures, 
    // each of which contains one filename and one inode number.
    map<string, fsInode*>  MainDir ; 
    // OpenFileDescriptors --  when you open a file, 
	// the operating system creates an entry to represent that file
    // This entry number is the file descriptor. 
    vector< FileDescriptor > OpenFileDescriptors;
    int file_descriptor_index = -1; // counter 
    int direct_enteris;
    int block_size;
    int freeDisk = DISK_SIZE;
    int maxFileSize = 0;

    public:
    // ------------------------------------------------------------------------
    fsDisk() {
        sim_disk_fd = fopen( DISK_SIM_FILE , "r+" );
        assert(sim_disk_fd);
        for (int i = 0 ; i < DISK_SIZE ; i++) {
            int ret_val = fseek ( sim_disk_fd , i , SEEK_SET );
            ret_val = fwrite( "\0", 1, 1, sim_disk_fd );
            assert(ret_val == 1);
        }
        fflush(sim_disk_fd);
    }
    // ------------------------------------------------------------------------
    void listAll() {
        int i = 0;    
        for ( auto it = begin (OpenFileDescriptors); it != end (OpenFileDescriptors); ++it) {
            cout << "index: " << i << ": FileName: " << it->getFileName() <<  " , isInUse: " << it->isInUse() << endl; 
            i++;
        }
        char bufy;
        cout << "Disk content: '" ;
        for (i=0; i < DISK_SIZE ; i++) {
            int ret_val = fseek ( sim_disk_fd , i , SEEK_SET );
            ret_val = fread(  &bufy , 1 , 1, sim_disk_fd );
             cout << bufy;              
        }
        cout << "'" << endl;
    }
 
    // ------------------------------------------------------------------------
    void fsFormat( int blockSize = 4, int direct_Enteris_ = 3 ) {
        // format the disk
        block_size = blockSize;
        direct_enteris = direct_Enteris_;
        BitVectorSize = DISK_SIZE / blockSize ;
        BitVector = new int [BitVectorSize];
        for (int i = 0 ; i < BitVectorSize ; i++)
            BitVector[i] = freeBit;
        cout << "FORMAT DISK: number of blocks: " << BitVectorSize << endl;
        maxFileSize = block_size * (direct_enteris + block_size);
        // detle old files: 
        OpenFileDescriptors.clear();
        MainDir.clear();
        is_formated = true;
    }

    // ------------------------------------------------------------------------
    int CreateFile(string fileName) {
        // create new file
        if(!is_formated){
            cout << "file hasn't been formmated" << endl;
            return -1; 
        }
            
        if(MainDir.find(fileName) != MainDir.end()){
            cout << "ERROR: file name already exist"  << endl;
            return -1;
        }
        fsInode *fileNode = new fsInode(block_size , direct_enteris);
        FileDescriptor fileD(fileName , fileNode); //create fd
        fileD.setInUse(true);
        OpenFileDescriptors.emplace_back(fileD);
        file_descriptor_index++;
        MainDir.insert(pair<string, fsInode*> (fileName, fileNode));
        return file_descriptor_index;
          
    }
    // ------------------------------------------------------------------------
    int OpenFile(string fileName) {
        // open a file that's been created
        if(MainDir.find(fileName) == MainDir.end()){
            cout << "ERROR: file not found" << endl;
            return -1;
        }
        int fd = 0; 
        for (fd = 0 ; fd < OpenFileDescriptors.size() ; fd ++){ // find the wanted file
            if (OpenFileDescriptors[fd].getFileName().compare(fileName) == 0){
                break;
            }
        }
        if(OpenFileDescriptors[fd].isInUse()){
            cout << "file already open" << endl;
            return fd;
        }
        OpenFileDescriptors[fd].setInUse(true); // open file
        return fd;
    }  

    // ------------------------------------------------------------------------
    string CloseFile(int fd) {
        if(OpenFileDescriptors.size() <= fd || OpenFileDescriptors[fd].getInode() == NULL){
            cout << "ERROR: file not found" << endl;
            return "-1";
        }
        if (!OpenFileDescriptors[fd].isInUse()){
            cout << "file already close" << endl;
            return OpenFileDescriptors[fd].getFileName();
        }
        OpenFileDescriptors[fd].setInUse(false); //close file
        return OpenFileDescriptors[fd].getFileName();

    }
    // ------------------------------------------------------------------------


    int WriteToFile(int fd, char *buf, int len ) {
        // check for error:
        if(!is_formated){
            cout<< "ERROR: disk hasn't been formated" << endl;
            return -1;
        }
        if(freeDisk == 0){
            cout<< "ERROR: disk is full" << endl;
            return -1;
        }
        if(OpenFileDescriptors.size() <= fd || MainDir.find(OpenFileDescriptors[fd].getFileName()) == MainDir.end()){
            cout<< "ERROR: file not found" << endl;
            return -1;
        }
        if(!OpenFileDescriptors[fd].isInUse()){
            cout<<"file not open" << endl;
            return -1;
        }
        int fileSize = OpenFileDescriptors[fd].getInode()->getFileSize();
        fsInode *fileInode = OpenFileDescriptors[fd].getInode();
        if (fileSize == maxFileSize){
            cout<< "ERROR: can't write to file, file is full" << endl;
            return -1;            
        }
        // where to write:
        int blockToWrite; // number of blocks to write in the disk
        int Len = min(len ,maxFileSize - fileSize); // actual length to write
        int sucssesWrite = Len; // return value
        int directSize = direct_enteris * block_size; // direct size in bytes
        int * directArr = fileInode->getDirectArr(); 
        int offset = 0; // offset in block
        int written = 0;
        int toWrite; // how much to write
        int moveCursor; // move cursor on disk file
        int ret_val;
        char buffer [block_size]; // temporary buffer to store bytes to write to a single block
        for (int j = 0 ; j < block_size && buf [j+written] != '\0' ; j ++){
            buffer[j] = buf[j+written];
        }
        //write to direct enteries:
        if (directSize > fileSize ){
            // in case there is a block not entirely full
            if (fileInode->freeBitInBlock() != 0){
                int b = fileInode->nextFreeDirectBlock(); // block's index
                blockToWrite = directArr[b];
                offset = block_size - fileInode->freeBitInBlock();
                toWrite = min (fileInode->freeBitInBlock(), Len);
                ret_val = fseek ( sim_disk_fd, block_size*blockToWrite + offset , SEEK_SET );
                assert(ret_val == 0);
                written += fwrite( buf, 1 , toWrite , sim_disk_fd );
                Len = Len - toWrite; // update the remainign length
                fileInode->addToFileSize(written); 
                if (Len == 0) // finished writing
                    return written;
                for (int j = 0 ; j < block_size && buf [j+written] != '\0' ; j ++){
                    buffer[j] = buf[j+written];
                }
            }
            //write to full block:
            int i = fileInode->nextFreeDirectBlock();
            for ( i ; i < direct_enteris ; i ++){
                blockToWrite = findFreeBlock();
                toWrite = min(Len, block_size);
                // write to block
                moveCursor = blockToWrite * block_size ;
                ret_val = fseek ( sim_disk_fd, moveCursor, SEEK_SET );
                assert(ret_val == 0);
                written += fwrite( buffer, 1 , toWrite , sim_disk_fd );
                //set direct array
                directArr[i] = blockToWrite;
                for (int j = 0 ; j < block_size && buf [j+written] != '\0' ; j ++){
                    buffer[j] = buf[j+written];
                }                
                // set len
                Len = Len - toWrite; 
                //set block in use
                fileInode->addToFileSize(toWrite);
                fileInode->setBlockInUse();
                if (Len == 0)
                    return sucssesWrite;
            }
        }

        // write to singlInDirect:
        int managBlock;
        int managBlockOffset = fileInode->getBlockInUse()-direct_enteris;
        char buf1 [1];
        char binBlock = '\0';
        
        if (fileInode->getSinglInDirect() == -1){
            managBlock = findFreeBlock();
            fileInode->setSinglInDirect(managBlock);
            for (int i = 0 ; i < block_size ; i ++){
                moveCursor = managBlock * block_size + i;
                ret_val = fseek ( sim_disk_fd, moveCursor, SEEK_SET );
                assert(ret_val == 0);
                buf1[0] = ' ';
                fwrite( buf1, 1 , 1 , sim_disk_fd );
            }
        }
        else{
            managBlock = fileInode->getSinglInDirect();
            // in case there is a block not entirely full:
            if(fileInode->freeBitInBlock() != 0){
                moveCursor = managBlock*block_size +managBlockOffset - 1;
                ret_val = fseek ( sim_disk_fd, moveCursor, SEEK_SET );
                assert(ret_val == 0);
                fread(buf1 , 1  ,1 , sim_disk_fd);
                binBlock = buf1[0];
                blockToWrite = (int)binBlock;
                offset = block_size - fileInode->freeBitInBlock();
                moveCursor = blockToWrite * block_size + offset;
                ret_val = fseek ( sim_disk_fd, moveCursor, SEEK_SET );
                assert(ret_val == 0);
                toWrite = min(Len , fileInode->freeBitInBlock());
                written += fwrite( buffer, 1 , toWrite , sim_disk_fd );
                Len = Len - toWrite;
                fileInode->addToFileSize(toWrite);
                fileInode->setBlockInUse();
                if (Len == 0)
                    return written;
                
            }
        }
        //write to full block:
        for (int i = managBlockOffset ; i < block_size ; i ++){
            // create new block in single indirect
            blockToWrite = findFreeBlock();
            decToBinary(blockToWrite , binBlock);
            moveCursor = managBlock * block_size + i;
            ret_val = fseek ( sim_disk_fd, moveCursor, SEEK_SET );
            assert(ret_val == 0);
            buf1[0] = binBlock;
            fwrite( buf1, 1 , 1 , sim_disk_fd );
            // write to the block
            moveCursor = blockToWrite * block_size;
            toWrite = min(block_size, Len);
            ret_val = fseek ( sim_disk_fd, moveCursor, SEEK_SET );
            assert(ret_val == 0);       
            written += fwrite( buffer, 1 , toWrite , sim_disk_fd );
            for (int j = 0 ; j < block_size && buf [j+written] != '\0' ; j ++){
                // update temporary buffer
                buffer[j] = buf[j+written];
            }
            // general updates
            Len = Len - toWrite;
            fileInode->addToFileSize(toWrite);
            fileInode->setBlockInUse();
            if (Len == 0)
                return written;
        }
//delete fileInode;
        freeDisk -= sucssesWrite;
        return sucssesWrite;
    }


    // ------------------------------------------------------------------------
    int DelFile( string FileName ) {
        if(MainDir.find(FileName) == MainDir.end()){
            cout << "ERROR: file not found" << endl;
            return -1;
        }
        int fd = 0; 
        // find fd
        for (fd = 0 ; fd < OpenFileDescriptors.size() ; fd ++){ // find the wanted file
            if (OpenFileDescriptors[fd].getFileName().compare(FileName) == 0){
                break;
            }
        }
        MainDir.erase(FileName); // erase from directory
        OpenFileDescriptors[fd].getInode()->getDirectArr();
        for (int i = 0 ; i < direct_enteris ; i++){ // clear blocks
            int btf = OpenFileDescriptors[fd].getInode()->getDirectArr()[i];
            if(btf != -1)
                BitVector[btf] = freeBit;  
        }
        int sid =  OpenFileDescriptors[fd].getInode()->getSinglInDirect();
        char buf[1];
        if (sid != -1){
            for (int i = 0 ; i < OpenFileDescriptors[fd].getInode()->getBlockInUse() - direct_enteris ; i++){
                int moveCursor = sid* block_size + i;
                int ret_val = fseek ( sim_disk_fd, moveCursor, SEEK_SET );
                assert(ret_val == 0);
                fread(buf , 1  ,1 , sim_disk_fd);
                char binBlock = buf[0];
                int blockDel = (int)binBlock;
                BitVector[blockDel] = freeBit;
            }

        }
        OpenFileDescriptors[fd].deleteFile();
        delete OpenFileDescriptors[fd].getInode();
        return fd;
    }
    // ------------------------------------------------------------------------
    int ReadFromFile(int fd, char *buf, int len ) { 
        if(!is_formated){
            cout<< "ERROR: disk hasn't been formated" << endl;
            return -1;
        }
        if(OpenFileDescriptors.size() <= fd || MainDir.find(OpenFileDescriptors[fd].getFileName()) == MainDir.end()){
            cout<< "ERROR: file not found" << endl;
            return -1;
        }
        if(!OpenFileDescriptors[fd].isInUse()){
            cout<<"file not open" << endl;
            return -1;
        }
        
        char buffer [maxFileSize]; // temporary buffer
        fsInode *fileInode = OpenFileDescriptors[fd].getInode();
        int * directArr = fileInode->getDirectArr();
        int Len = min(len , fileInode->getFileSize());
        int succsessRead = 0;
        int blockToRead;
        int i;
        int moveCursor;
        int toRead;
        int ret_val;
        buf[Len] = '\0';
        // read from direct :
        for ( i = 0 ; i < direct_enteris && directArr[i] != -1 ; i++){
            blockToRead = directArr[i];
            moveCursor = blockToRead * block_size ;
            toRead = min (block_size , Len); // actuall size to read for a single block
            ret_val = fseek ( sim_disk_fd, moveCursor, SEEK_SET );
            assert(ret_val == 0);
            succsessRead += fread(buffer , 1  ,toRead , sim_disk_fd);
            for (int j = 0 ; j < toRead ; j++){ // update buf
                buf[j +((succsessRead- toRead))] = buffer[j];
            }
            Len = Len- toRead;
            if (Len == 0)
                return succsessRead;
        }
        // read from single indirect:
        char binBlock;
        int managBlock = fileInode->getSinglInDirect();
        char buf1[1];
        for(int i = 0 ; i < block_size ; i ++){
            // find block to read from:
            moveCursor = managBlock* block_size + i;
            ret_val = fseek ( sim_disk_fd, moveCursor, SEEK_SET );
            assert(ret_val == 0);
            fread(buf1 , 1  ,1 , sim_disk_fd);
            binBlock = buf1[0];
            blockToRead = (int)binBlock;
            // read from block:
            moveCursor = blockToRead*block_size;
            toRead = min(Len, block_size);
            ret_val = fseek ( sim_disk_fd, moveCursor, SEEK_SET );
            assert(ret_val == 0);
            succsessRead += fread(buffer , 1  ,toRead , sim_disk_fd);
            // general updates
            for (int j = 0 ; j < toRead ; j++){
                buf[j +((succsessRead- toRead))] = buffer[j];
            }
            Len = Len- toRead;
            if (Len == 0)
                return succsessRead;
        }
        return succsessRead;
        
    }
    // ------------------------------------------------------------------------
    int findFreeBlock(){ // finds a free block in the disk
        int i = 0;
        for (i = 0 ; i < BitVectorSize ; i++){
            if (BitVector[i] == freeBit)
                break;
            }
        if (i == BitVectorSize)
            return -1;
        BitVector[i] = occupied;
          return i;
      }
      ~fsDisk (){
            fclose(sim_disk_fd);
            for (int i = 0 ; i < OpenFileDescriptors.size() ; i ++){
                delete OpenFileDescriptors[i].getInode();
            }
            delete BitVector;
      }
};
    
int main() {
    int blockSize; 
	int direct_entries;
    string fileName;
    char str_to_write[DISK_SIZE];
    char str_to_read[DISK_SIZE];
    int size_to_read; 
    int _fd;

    fsDisk *fs = new fsDisk();
    int cmd_;
    while(1) {
        cin >> cmd_;
        switch (cmd_)
        {
            case 0:   // exit
				delete fs;
				exit(0);
                break;

            case 1:  // list-file
                fs->listAll(); 
                break;
          
            case 2:    // format
                cin >> blockSize;
				cin >> direct_entries;
                fs->fsFormat(blockSize, direct_entries);
                break;
          
            case 3:    // creat-file
                cin >> fileName;
                _fd = fs->CreateFile(fileName);
                cout << "CreateFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;
            
            case 4:  // open-file
                cin >> fileName;
                _fd = fs->OpenFile(fileName);
                cout << "OpenFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;
             
            case 5:  // close-file
                cin >> _fd;
                fileName = fs->CloseFile(_fd); 
                cout << "CloseFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;
           
            case 6:   // write-file
                cin >> _fd;
                cin >> str_to_write;
                fs->WriteToFile( _fd , str_to_write , strlen(str_to_write) );
                break;
          
            case 7:    // read-file
                cin >> _fd;
                cin >> size_to_read ;
                fs->ReadFromFile( _fd , str_to_read , size_to_read );
                cout << "ReadFromFile: " << str_to_read << endl;
                break;
           
            case 8:   // delete file 
                 cin >> fileName;
                _fd = fs->DelFile(fileName);
                cout << "DeletedFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;
            default:
                break;
        }
    }

} 