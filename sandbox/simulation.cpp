#include "engine.h"
#include<vector>
#include<ctime>
using std::cout ;

int main(int argc, char* argv[]) {

	int start_time = time(0); 
	init(); 
	double pct_acc = 0; 
	char progress[50] ; 
	sprintf(progress, "RP = %g \t TP = %g \t RPTP = %g ", getRP(),getTP(),getRPTP() ) ; 
	fputs(progress,progressFile) ;
	
	for(int wi = 0; wi < numWindows; wi ++ ) {

		for(int wpass = 0; wpass < numPasses; wpass ++) {
			//adjustZ(wi); 

			for(int j = 0; j < numSteps; j++) { 

				int acc = mc_step(wi); 
				pct_acc += acc / float(numFrames) ; 
				
				
				if(j > equilibrationTime and j%sampleRate == 0  ) WriteEventData(j,wi);	
				//if(j > equilibrationTime and j%sampleRate == 0  ) cout << getZ() << endl ;	
				
				
				// Write progress report 
				if(j%progressRate == 0 ) { 
					
					//char progress[50]; 
					sprintf(progress,"window %d/%d\tpass %g/%g\tstep %g/%g\n",wi+1,numWindows,(float) wpass+1,(float) numPasses,(float) j , (float) numSteps);
					fputs(progress,progressFile);
				}
			checkNorms(); 

			} // end simulation 
		
		} // end pass 

	}	// end window 


	// write statistics and close up 
	writeHistograms();
	write_metadata(); 
	
	sprintf(progress, "Z = %g \t RP = %g \t TP = %g \t RPTP = %g ", getZ() ,getRP(),getTP(),getRPTP() ) ; 
	fputs(progress,progressFile) ;

	char summary[70];
	int end_time = time(0); 
	double tdiff = (end_time - start_time)/float(60); // simulation time in minutes  
	sprintf(summary,"%.1f%% steps accepted\nrunning time: %.2f minutes\n",100*(pct_acc),tdiff);
	fputs(summary,progressFile); 

	return cleanup(); 
}



