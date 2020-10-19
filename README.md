# Delegated PSI Cardinality  &  Contact Tracing
This is the implementation of our [Asiacryp 2020](https://asiacrypt.iacr.org/2020/)  paper: **Catalic: Delegated PSI Cardinality withApplications to Contact Tracing**[[ePrint](https://eprint.iacr.org/2020/1105)]. 

## Installations

### Required libraries
 C++ compiler with C++14 support. There are several library dependencies including [`Boost`](https://sourceforge.net/projects/boost/), [`Miracl`](https://github.com/miracl/MIRACL), and [`NTL`](http://www.shoup.net/ntl/)
 Optional: `nasm` for improved SHA1 performance.   Our code has been tested on both Windows (Microsoft Visual Studio) and Linux. To install the required libraries: 
  * windows: open PowerShell,  `cd ./thirdparty`, and `.\all_win.ps1` 
  * linux: `cd ./thirdparty`, and `bash .\all_linux.get`.   

## Running the code
The database is generated randomly. The outputs include the average online/offline/total runtime (ms) that displayed on the screen. 
#### Flags:
	-m		number of cloud servers
	-p		polynomial-based construction (estimating peformance of polynomial operations)
	-g		GBF-based construction (estimating peformance of GBF operations)
	-c		client's performance
#### Examples: 
     ./bin/frontend.exe -m 32 -p  
     
## Help
For any questions on building or running the library, please contact [`Ni Trieu`](http://people.oregonstate.edu/~trieun/) at trieun at oregonstate dot edu     