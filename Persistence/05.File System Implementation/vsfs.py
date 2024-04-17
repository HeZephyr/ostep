#! /usr/bin/env python

from __future__ import print_function
import random
from optparse import OptionParser

# to make Python2 and Python3 act the same -- how dumb
def random_seed(seed):
    try:
        random.seed(seed, version=1)
    except:
        random.seed(seed)
    return

DEBUG = False

def dprint(str):
    if DEBUG:
        print(str)

printOps      = True
printState    = True
printFinal    = True

class bitmap:
    """
    Represents a bitmap used for tracking allocated/free blocks in a file system.

    Attributes:
        size (int): The size of the bitmap (number of blocks).
        bmap (list): List representing the bitmap where 0 indicates a free block and 1 indicates an allocated block.
        numAllocated (int): The number of currently allocated blocks.
    """
    def __init__(self, size):
        self.size = size
        self.bmap = []
        self.bmap = [0] * size  # Initialize the bitmap with all blocks initially free
        self.numAllocated = 0   # Initially, no blocks are allocated

    # Find the first free block in the bitmap and allocate it.
    def alloc(self):
        for num in range(len(self.bmap)):
            if self.bmap[num] == 0:
                self.bmap[num] = 1
                self.numAllocated += 1
                return num
        return -1

    # Frees a block
    def free(self, num):
        assert(self.bmap[num] == 1)
        self.numAllocated -= 1
        self.bmap[num] = 0

    # Marks a block as allocated
    def markAllocated(self, num):
        assert(self.bmap[num] == 0)
        self.numAllocated += 1
        self.bmap[num] = 1

    # Returns the number of free blocks in the bitmap
    def numFree(self):
        return self.size - self.numAllocated

    # Returns a string representation of the bitmap.
    def dump(self):
        s = ''
        for i in range(len(self.bmap)):
            s += str(self.bmap[i])
        return s

class block:
    """
    Represents a block in a file system, which can be either a directory ('d'), a regular file ('f'), or free.

    Attributes:
        ftype (str): The type of the block ('d' for directory, 'f' for file, 'free' for free block).
        dirUsed (int): The number of directory entries used if the block represents a directory.
        maxUsed (int): The maximum number of directory entries that can be accommodated in the block.
        dirList (list): List of directory entries if the block represents a directory. Each entry is 
        a tuple of the form ('name', inum), where 'name' is the name of the entry and 'inum' is the corresponding inode number.
        data (str): The data stored in the block if it represents a regular file.
    """
    def __init__(self, ftype):
        assert(ftype == 'd' or ftype == 'f' or ftype == 'free')
        self.ftype = ftype
        # only for directories, properly a subclass but who cares
        self.dirUsed = 0
        self.maxUsed = 32
        self.dirList = []
        self.data    = ''

    # Returns a string representation of the block.
    def dump(self):
        if self.ftype == 'free':
            return '[]'
        elif self.ftype == 'd':
            rc = ''
            for d in self.dirList:
                # d is of the form ('name', inum)
                short = '(%s,%s)' % (d[0], d[1])
                if rc == '':
                    rc = short
                else:
                    rc += ' ' + short
            return '['+rc+']'
            # return '%s' % self.dirList
        else:
            return '[%s]' % self.data

    def setType(self, ftype):
        assert(self.ftype == 'free')
        self.ftype = ftype

    def addData(self, data):
        assert(self.ftype == 'f')
        self.data = data

    # Returns the number of directory entries in the block.
    def getNumEntries(self):
        assert(self.ftype == 'd')
        return self.dirUsed

    # Returns the number of free directory entries in the block.
    def getFreeEntries(self):
        assert(self.ftype == 'd')
        return self.maxUsed - self.dirUsed

    # Returns the directory entry at the specified index.
    def getEntry(self, num):
        assert(self.ftype == 'd')
        assert(num < self.dirUsed)
        return self.dirList[num]

    # Adds a directory entry to the block.
    def addDirEntry(self, name, inum):
        assert(self.ftype == 'd')
        self.dirList.append((name, inum))
        self.dirUsed += 1
        assert(self.dirUsed <= self.maxUsed)

    # Deletes a directory entry from the block.
    def delDirEntry(self, name):
        assert(self.ftype == 'd')
        tname = name.split('/')
        dname = tname[len(tname) - 1]
        for i in range(len(self.dirList)):
            if self.dirList[i][0] == dname:
                self.dirList.pop(i)
                self.dirUsed -= 1
                return
        assert(1 == 0)

    # Checks if a directory entry exists in the block.
    def dirEntryExists(self, name):
        assert(self.ftype == 'd')
        for d in self.dirList:
            if name == d[0]:
                return True
        return False

    # Frees the block by resetting its attributes.
    def free(self):
        assert(self.ftype != 'free')
        if self.ftype == 'd':
            # check for only dot, dotdot here
            assert(self.dirUsed == 2)
            self.dirUsed = 0
        self.data  = ''
        self.ftype = 'free'

class inode:
    """
    Represents an inode in a file system, which contains metadata about a file or directory.

    Attributes:
        ftype (str): The type of the inode ('d' for directory, 'f' for file, 'free' for free inode).
        addr (int): The address of the block containing the data associated with the inode.
        refCnt (int): The reference count of the inode, indicating the number of links pointing to it.
    """
    def __init__(self, ftype='free', addr=-1, refCnt=1):
        self.setAll(ftype, addr, refCnt)

    def setAll(self, ftype, addr, refCnt):
        assert(ftype == 'd' or ftype == 'f' or ftype == 'free')
        self.ftype  = ftype
        self.addr   = addr
        self.refCnt = refCnt

    def incRefCnt(self):
        self.refCnt += 1

    def decRefCnt(self):
        self.refCnt -= 1

    def getRefCnt(self):
        return self.refCnt

    def setType(self, ftype):
        assert(ftype == 'd' or ftype == 'f' or ftype == 'free')
        self.ftype = ftype

    def setAddr(self, block):
        self.addr = block

    def getSize(self):
        if self.addr == -1:
            return 0
        else:
            return 1

    def getAddr(self):
        return self.addr

    def getType(self):
        return self.ftype

    def free(self):
        self.ftype = 'free'
        self.addr  = -1
        

class fs:
    """
    Represents a simple file system.

    Attributes:
        numInodes (int): The total number of inodes in the file system.
        numData (int): The total number of data blocks in the file system.
        ibitmap (bitmap): An instance of the 'bitmap' class for managing inode allocation.
        inodes (list): A list of inode objects representing the inodes in the file system.
        dbitmap (bitmap): An instance of the 'bitmap' class for managing data block allocation.
        data (list): A list of block objects representing the data blocks in the file system.
        ROOT (int): The inode number of the root directory.
        files (list): A list of file names in the file system.
        dirs (list): A list of directory names in the file system.
        nameToInum (dict): A dictionary mapping file or directory names to their corresponding inode numbers.
    """
    def __init__(self, numInodes, numData):
        self.numInodes = numInodes
        self.numData   = numData
        
        self.ibitmap = bitmap(self.numInodes)
        self.inodes = [inode() for _ in range(self.numInodes)]

        self.dbitmap = bitmap(self.numData)
        self.data    = [block('free') for _ in range(self.numData)]
        # root inode
        self.ROOT = 0

        # create root directory
        self.ibitmap.markAllocated(self.ROOT)
        self.inodes[self.ROOT].setAll('d', 0, 2)
        self.dbitmap.markAllocated(self.ROOT)
        self.data[0].setType('d')

        # add . and .. to root directory
        self.data[0].addDirEntry('.',  self.ROOT)
        self.data[0].addDirEntry('..', self.ROOT)

        # these is just for the fake workload generator
        self.files      = []
        self.dirs       = ['/']

        # map of name to inum
        self.nameToInum = {'/':self.ROOT}

    # Prints the state of the file system.
    def dump(self):
        print('inode bitmap ', self.ibitmap.dump())
        print('inodes       ', end='')
        for i in range(0,self.numInodes):
            ftype = self.inodes[i].getType()
            if ftype == 'free':
                print('[]', end='')
            else:
                print('[%s a:%s r:%d]' % (ftype, self.inodes[i].getAddr(), self.inodes[i].getRefCnt()), end='')
        print('')
        print('data bitmap  ', self.dbitmap.dump())
        print('data         ', end='')
        for i in range(self.numData):
            print(self.data[i].dump(), end='')
        print('')

    # Generates a random name for a file or directory.
    def makeName(self):
        p = ['a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'j', 'k', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z']
        return p[int(random.random() * len(p))]

    # Allocates an unused inode.
    def inodeAlloc(self):
        return self.ibitmap.alloc()

    # Frees an inode.
    def inodeFree(self, inum):
        self.ibitmap.free(inum)
        self.inodes[inum].free()

    # Allocates an unused data block.
    def dataAlloc(self):
        return self.dbitmap.alloc()

    # Frees a data block.
    def dataFree(self, bnum):
        self.dbitmap.free(bnum)
        self.data[bnum].free()
    
    # Returns the parent directory path of a file or directory.
    def getParent(self, name):
        tmp = name.split('/')
        if len(tmp) == 2:
            return '/'
        pname = ''
        for i in range(1, len(tmp)-1):
            pname = pname + '/' + tmp[i]
        return pname

    # Delete the specified file
    def deleteFile(self, tfile):
        if printOps:
            print('unlink("%s");' % tfile)

        # get the inode number
        inum = self.nameToInum[tfile]
        ftype = self.inodes[inum].getType()

        if self.inodes[inum].getRefCnt() == 1:
            # free data blocks first
            dblock = self.inodes[inum].getAddr()
            # free data block
            if dblock != -1:
                self.dataFree(dblock)
            # then free inode
            self.inodeFree(inum)
        else:
            self.inodes[inum].decRefCnt()

        # remove from parent directory
        parent = self.getParent(tfile)
        # print('--> delete from parent', parent)
        pinum = self.nameToInum[parent]
        # print('--> delete from parent inum', pinum)
        pblock = self.inodes[pinum].getAddr()
        # FIXED BUG: DECREASE PARENT INODE REF COUNT if needed (thanks to Srinivasan Thirunarayanan)
        if ftype == 'd':
            self.inodes[pinum].decRefCnt()
        # print('--> delete from parent addr', pblock)
        self.data[pblock].delDirEntry(tfile)

        # finally, remove from files list
        self.files.remove(tfile)
        return 0

    # Create a new file or directory under the specified parent directory    
    def createLink(self, target, newfile, parent):
        # find info about parent
        parentInum = self.nameToInum[parent]

        # is there room in the parent directory?
        pblock = self.inodes[parentInum].getAddr()
        if self.data[pblock].getFreeEntries() <= 0:
            dprint('*** createLink failed: no room in parent directory ***')
            return -1

        # print('is %s in directory %d' % (newfile, pblock))
        if self.data[pblock].dirEntryExists(newfile):
            dprint('*** createLink failed: not a unique name ***')
            return -1

        # Find the inode number of the target file and increment its reference count
        tinum = self.nameToInum[target]
        self.inodes[tinum].incRefCnt()

        # DONT inc parent ref count - only for directories
        # self.inodes[parentInum].incRefCnt()

        # now add to directory
        tmp = newfile.split('/')
        # get the file name
        ename = tmp[len(tmp)-1]
        self.data[pblock].addDirEntry(ename, tinum)
        return tinum

    # Create a new file or directory under the specified parent directory.
    def createFile(self, parent, newfile, ftype):
        # find info about parent
        parentInum = self.nameToInum[parent]

        # is there room in the parent directory?
        pblock = self.inodes[parentInum].getAddr()
        if self.data[pblock].getFreeEntries() <= 0:
            dprint('*** createFile failed: no room in parent directory ***')
            return -1

        # have to make sure file name is unique
        block = self.inodes[parentInum].getAddr()
        # print('is %s in directory %d' % (newfile, block))
        if self.data[block].dirEntryExists(newfile):
            dprint('*** createFile failed: not a unique name ***')
            return -1
        
        # find free inode and allocate
        inum = self.inodeAlloc()
        if inum == -1:
            dprint('*** createFile failed: no inodes left ***')
            return -1
        
        # if a directory, have to allocate directory block for basic (., ..) info
        fblock = -1
        if ftype == 'd':
            refCnt = 2
            fblock = self.dataAlloc()
            if fblock == -1:
                dprint('*** createFile failed: no data blocks left ***')
                self.inodeFree(inum)
                return -1
            else:
                self.data[fblock].setType('d')
                self.data[fblock].addDirEntry('.',  inum)
                self.data[fblock].addDirEntry('..', parentInum)
        else:
            refCnt = 1
            
        # now ok to init inode properly
        self.inodes[inum].setAll(ftype, fblock, refCnt)

        # inc parent ref count if this is a directory
        if ftype == 'd':
            self.inodes[parentInum].incRefCnt()

        # and add to directory of parent
        self.data[pblock].addDirEntry(newfile, inum)
        return inum

    # Write data to the specified file.
    def writeFile(self, tfile, data):
        inum = self.nameToInum[tfile]
        # get current size of file
        curSize = self.inodes[inum].getSize()
        dprint('writeFile: inum:%d cursize:%d refcnt:%d' % (inum, curSize, self.inodes[inum].getRefCnt()))
        if curSize == 1:
            dprint('*** writeFile failed: file is full ***')
            return -1
        # # Allocate a data block for writing the data
        fblock = self.dataAlloc()
        if fblock == -1:
            dprint('*** writeFile failed: no data blocks left ***')
            return -1
        else:
            self.data[fblock].setType('f')
            self.data[fblock].addData(data)
        self.inodes[inum].setAddr(fblock)
        if printOps:
            print('fd=open("%s", O_WRONLY|O_APPEND); write(fd, buf, BLOCKSIZE); close(fd);' % tfile)
        return 0
    
    # Perform a file deletion operation.
    def doDelete(self):
        dprint('doDelete')
        if len(self.files) == 0:
            return -1
        dfile = self.files[int(random.random() * len(self.files))]
        dprint('try delete(%s)' % dfile)
        return self.deleteFile(dfile)

    # Perform a file link operation.
    def doLink(self):
        dprint('doLink')
        if len(self.files) == 0:
            return -1
        parent = self.dirs[int(random.random() * len(self.dirs))]
        nfile = self.makeName()

        # pick random target
        target = self.files[int(random.random() * len(self.files))]
        # MUST BE A FILE, not a DIRECTORY (this will always be true here)

        # get full name of newfile
        if parent == '/':
            fullName = parent + nfile
        else:
            fullName = parent + '/' + nfile

        dprint('try createLink(%s %s %s)' % (target, nfile, parent))
        inum = self.createLink(target, nfile, parent)
        if inum >= 0:
            self.files.append(fullName)
            self.nameToInum[fullName] = inum
            if printOps:
                print('link("%s", "%s");' % (target, fullName))
            return 0
        return -1
    
    # Perform a file creation operation.
    def doCreate(self, ftype):
        dprint('doCreate')
        parent = self.dirs[int(random.random() * len(self.dirs))]
        nfile = self.makeName()
        if ftype == 'd':
            tlist = self.dirs
        else:
            tlist = self.files

        if parent == '/':
            fullName = parent + nfile
        else:
            fullName = parent + '/' + nfile

        dprint('try createFile(%s %s %s)' % (parent, nfile, ftype))
        inum = self.createFile(parent, nfile, ftype)
        if inum >= 0:
            tlist.append(fullName)
            self.nameToInum[fullName] = inum
            if parent == '/':
                parent = ''
            if ftype == 'd':
                if printOps:
                    print('mkdir("%s/%s");' % (parent, nfile))
            else:
                if printOps:
                    print('creat("%s/%s");' % (parent, nfile))
            return 0
        return -1

    # Perform a file append operation.
    def doAppend(self):
        dprint('doAppend')
        if len(self.files) == 0:
            return -1
        afile = self.files[int(random.random() * len(self.files))]
        dprint('try writeFile(%s)' % afile)
        data = chr(ord('a') + int(random.random() * 26))
        rc = self.writeFile(afile, data)
        return rc

    # Run a sequence of file system operations.
    def run(self, numRequests):
        # set up initial state
        self.percentMkdir  = 0.40
        self.percentWrite  = 0.40
        self.percentDelete = 0.20
        self.numRequests   = 20

        print('Initial state')
        print('')
        self.dump()
        print('')
        
        for _ in range(numRequests):
            if printOps == False:
                print('Which operation took place?')
            rc = -1
            while rc == -1:
                r = random.random()
                if r < 0.3:
                    rc = self.doAppend()
                    dprint('doAppend rc:%d' % rc)
                elif r < 0.5:
                    rc = self.doDelete()
                    dprint('doDelete rc:%d' % rc)
                elif r < 0.7:
                    rc = self.doLink()
                    dprint('doLink rc:%d' % rc)
                else:
                    if random.random() < 0.75:
                        rc = self.doCreate('f')
                        dprint('doCreate(f) rc:%d' % rc)
                    else:
                        rc = self.doCreate('d')
                        dprint('doCreate(d) rc:%d' % rc)
                if self.ibitmap.numFree() == 0:
                    print('File system out of inodes; rerun with more via command-line flag?')
                    exit(1)
                if self.dbitmap.numFree() == 0:
                    print('File system out of data blocks; rerun with more via command-line flag?')
                    exit(1)
            if printState == True:
                print('')
                self.dump()
                print('')
            else:
                print('')
                print('  State of file system (inode bitmap, inodes, data bitmap, data)?')
                print('')

        if printFinal:
            print('')
            print('Summary of files, directories::')
            print('')
            print('  Files:      ', self.files)
            print('  Directories:', self.dirs)
            print('')

# main program
if __name__ == '__main__':
    parser = OptionParser()

    parser.add_option('-s', '--seed',        default=0,     help='the random seed',                      action='store', type='int', dest='seed')
    parser.add_option('-i', '--numInodes',   default=8,     help='number of inodes in file system',      action='store', type='int', dest='numInodes') 
    parser.add_option('-d', '--numData',     default=8,     help='number of data blocks in file system', action='store', type='int', dest='numData') 
    parser.add_option('-n', '--numRequests', default=10,    help='number of requests to simulate',       action='store', type='int', dest='numRequests')
    parser.add_option('-r', '--reverse',     default=False, help='instead of printing state, print ops', action='store_true',        dest='reverse')
    parser.add_option('-p', '--printFinal',  default=False, help='print the final set of files/dirs',    action='store_true',        dest='printFinal')
    parser.add_option('-c', '--compute',     default=False, help='compute answers for me',               action='store_true',        dest='solve')

    (options, args) = parser.parse_args()

    print('ARG seed',        options.seed)
    print('ARG numInodes',   options.numInodes)
    print('ARG numData',     options.numData)
    print('ARG numRequests', options.numRequests)
    print('ARG reverse',     options.reverse)
    print('ARG printFinal',  options.printFinal)
    print('')

    random_seed(options.seed)

    if options.reverse:
        printState = False
        printOps   = True
    else:
        printState = True
        printOps   = False

    if options.solve:
        printOps   = True
        printState = True

    printFinal = options.printFinal

    #
    # have to generate RANDOM requests to the file system
    # that are VALID!
    #

    f = fs(options.numInodes, options.numData)

    #
    # ops: mkdir rmdir : create delete : append write
    #

    f.run(options.numRequests)

