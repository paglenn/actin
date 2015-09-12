#include<iostream>
#include<cmath>
#include<vector>
#include<string>
#include<cstdlib>
#include<random>
#include<ctime>
using std::vector; 
using std::endl; 
using std::cout;
using std::cerr; 
using std::normal_distribution;

// system parameters 
double L,_L,ds, _ds; 
int N; 
vector<double> tx, ty, tz, t0x,t0y,t0z, tox,toy,toz ; 

// simulation parameters 
int numSweeps, numSteps; 
int numPasses, numWindows, numFrames; 
int sampleRate, progressRate, equilibrationTime; 
int nbins ; 
double binWidth, binOverlap; 
double gaussVar, bias ; 
vector<double> winMin, winMax, zmin,K ; 
double zstart, wmax,wmin,width, wmean, z ;
double target, tol;
int random_index ;

// hard conditions on certain variables
bool flag ; 
double RP, TP, RPTP ; 

// Data files 
FILE * inFile; 
FILE * zFile;
FILE * progressFile; 
FILE * metaFile; 
vector<FILE*> windowFiles; 
vector<FILE*> histFiles; 
vector< vector<double> > zHists; 
vector<int> totalCounts; 

const double PI = acos(-1.);
int iseed ; 
std::mt19937 generator; 

void readInParameters() {

	inFile = fopen("parameters.txt","r"); 
	if (inFile == NULL) { 
		cerr << "parameter file not found!" << endl; 
		exit(1); 
	}
	
	while(!feof(inFile)) {
		char str[50]; 
		fscanf(inFile,"%s",str);
		if ( *str == '#')  fgets(str,100,inFile) ; 
		else if ( *str == 'L') fscanf(inFile,"%lf",&L);
		else if ( *str == 'N') fscanf(inFile,"%d", &N); 
		else if (!strcmp(str,"numSweeps")) fscanf(inFile,"%d",&numSweeps); 
		else if (!strcmp(str,"sampleRate")) fscanf(inFile,"%d",&sampleRate); 
		else if (!strcmp(str,"progressRate")) fscanf(inFile,"%d",&progressRate); 
		else if (!strcmp(str,"equilibrationTime")) fscanf(inFile,"%d",&equilibrationTime); 
		else if (!strcmp(str,"iseed"))	fscanf(inFile, "%d", &iseed ) ;  
		else if (!strcmp(str,"gaussVar")) fscanf(inFile,"%lf",&gaussVar); 
		else if (!strcmp(str,"numWindows")) fscanf(inFile,"%d",&numWindows); 
		else if (!strcmp(str,"numPasses")) fscanf(inFile,"%d",&numPasses); 
		else if (!strcmp(str,"zstart")) fscanf(inFile,"%lf",&zstart); 
		else if (!strcmp(str,"bias")) fscanf(inFile,"%lf",&bias); 
		else if (!strcmp(str,"nbins")) fscanf(inFile,"%d",&nbins); 
		//else std::cout<<"whoami" << str<<std::endl;
	}

	// MF correction 
	_L = 1./L ; 
	L = 1./(_L + 2) ; 
}

void createHistograms() {

	totalCounts = vector<int>(numWindows,0) ; 
	for(int i = 0.; i < numWindows; i++) {
		zHists.push_back( vector<double>(nbins, 0.0) ) ; 
	}
}

void normalize(int index) {

	double norm = sqrt(tx[index]*tx[index] + ty[index]*ty[index] + tz[index]*tz[index] );
	tx[index] /= norm; 
	ty[index] /= norm; 
	tz[index] /= norm; 

}

double getZ() {
	// height of polymer along z-axis  
	double zsum  = 0.;
	for(int j = 0; j < N; j++) zsum += tz[j] ;
	return zsum/(float) N; 
}

double getRP() {
	// projection of eed vector into the plane 
	double rpx = 0.,rpy = 0.;
	for(int j = 0; j < N; j++) {
		rpx += tx[j]; 
		rpy += ty[j];
	}
	double rp = sqrt(rpx*rpx+rpy*rpy);
	return rp/float(N);
}

double getRPTP() {
	// innerProduct product of perpendicular component of final segment and 
	// projection of eed vector into the plane 
	double rx = 0.,ry = 0.; 
	double tpx=0.,tpy=0.;
	tpx = tx[N-1]; tpy = ty[N-1]; 
	for(int i = 1; i < N; i++) {
		rx += tx[i];
		ry += ty[i];
	}

	return (rx*tpx + ry*tpy)/ (double) N; 
}

	
double getTP() { 
	//perpendicular component of final segment 
	double tp = sqrt(tx[N-1]*tx[N-1] + ty[N-1]*ty[N-1]); 
	return tp; 
}


void init() {

	readInParameters();
//	std::cout<<"Initialized" << std::endl;

	ds = L/ (double) N ; 
	_ds  = 1./ds; 
	numSteps = numSweeps * N; 
	cout << numSteps << endl ;
	gaussVar *= ds; 
	numFrames = numWindows * numPasses * numSteps ; 
	binWidth = (1. - zstart)/nbins; 
	binOverlap = 1 * binWidth; 
	iseed = time(0) ; 
	srand(iseed); 
	generator.seed(iseed); 

	zFile = fopen("uwham.dat","w");

	progressFile = fopen("progress.out","w"); 
	if ( progressRate > numFrames/1000 ) progressRate = numFrames / 1000 ; 

	tx.assign(N,0.); 
	ty.assign(N,0.); 
	tz.assign(N,1.);
	t0x.assign(N,0.); 
	t0y.assign(N,0.); 
	t0z.assign(N,1.);
	tox.assign(N,0.); 
	toy.assign(N,0.); 
	toz.assign(N,1.);
	// initialize to a random configuration 
	// don't change first ti from (0,0,1) 
	// limit to ti > 0 at first 
	z = -1;
	while ( z  < 0 ) { 	
	for (int index = 1 ; index < N ; index++ ) { 

		normal_distribution<double> gaus(0.0,1);

		tx[index] = gaus(generator);
		gaus.reset(); // needed for independent rv's 
		ty[index] = gaus(generator);
		gaus.reset();
		while( tz[index] <= 0. ) { 
			tz[index] = gaus(generator);
			gaus.reset();
		}

		normalize(index);
		t0x[index] = tx[index]; 
		tox[index] = tx[index];
		t0y[index] = ty[index]; 
		toy[index] = ty[index]; 
		t0z[index] = tz[index];
		toz[index] = tz[index];
	}
		z = getZ(); 
	}

	RP = getRP() ; 
	RPTP = getRPTP() ; 
	TP = getTP() ; 
	cout << "Starting Z: " << z << endl; 
	cout << "RP " << RP << endl ; 
	cout << "TP " << TP << endl; 
	cout << "RPTP " << RPTP << endl ; 

	
	for(int j = 0.; j < numWindows; j++ ) {
		double wmax = ( zstart + (1-zstart) * double(j+1)/numWindows) ;
		double wmin = ( zstart + (1-zstart) * double(j)/numWindows) ;
		winMin.push_back( wmin ) ; 
		winMax.push_back( wmax ) ;
		double min_loc = 0.5*(wmax + wmin);
		zmin.push_back(min_loc); 

		K.push_back(bias); 

		char fileName[15]; 
		sprintf(fileName,"window_%d",j); 
		windowFiles.push_back(fopen(fileName,"w")); 
		
		sprintf(fileName,"hist_%d",j); 
		histFiles.push_back(fopen(fileName,"w")); 
	}

	createHistograms(); 
	
}

double V_bias(double z, int wj ) { 
	return 0.5*K[wj]*pow(z - zmin[wj], 2.);
}
/*
void normalize(int index) {

	double norm = sqrt(tx[index]*tx[index] + ty[index]*ty[index] + tz[index]*tz[index] );
	tx[index] /= norm; 
	ty[index] /= norm; 
	tz[index] /= norm; 

}
*/
void perturb(int index) { 

	tox[index] = tx[index]; 
	toy[index] = ty[index]; 
	toz[index] = tz[index]; 

	normal_distribution<double> gaus(0.0,gaussVar);

	tx[index] += gaus(generator);
	gaus.reset(); // needed for independent rv's 
	ty[index] += gaus(generator);
	gaus.reset();
	tz[index] += gaus(generator);
	gaus.reset();

	normalize(index);
}

double innerProduct(double x1, double y1, double z1, double x2, double y2, double z2) {
	double ans = x1*x2 + y1*y2 + z1*z2; 
	return ans; 
}


double deltaE(int index, int w_index, double z_prev) {
	
	double E_old = 0, E_new = 0  ; 
	
	if( index+1 != N) {
		E_old -= innerProduct(tx[index+1],ty[index+1],tz[index+1],tox[index],toy[index],toz[index]) ;
		E_new -= innerProduct(tx[index+1],ty[index+1],tz[index+1],tx[index],ty[index],tz[index]) ;
	}
	
	if( index != 0) {
		E_old -= innerProduct(tx[index-1],ty[index-1],tz[index-1],tox[index],toy[index],toz[index]) ;
		E_new -= innerProduct(tx[index-1],ty[index-1],tz[index-1],tx[index],ty[index],tz[index]) ;
	}

	double z_new = getZ(); 
	//double dVz = ( z_new < winMin[w_index] || z_new > winMax[w_index] ) ? bigNum : 0.;
	double dVz  = V_bias(z_new,w_index) - V_bias(z_prev,w_index) ; 


	return _ds*( E_new - E_old) + dVz;

}

double revert(int index) {

	tx[index] = tox[index]; 
	ty[index] = toy[index]; 
	tz[index] = toz[index]; 

	return 0; 

}

bool diff(double a, double b) { 
	if (fabs(a-b)  > 1e-6) return true; 
	else return false; 
}

void adjustZ(int w_index) {

	double wmean = zmin[w_index] ; 
	double z = 0.; 
	double target = wmean ; // (-1,1)
	double tol = ds;

	// this is bound to be horribly slow 
	// PROBLEM
	while(fabs(z - target) > tol || diff(RP,getRP()) || diff(TP,getTP()) || diff(RPTP, getRPTP() ) ) {

		random_index = 1+rand()%(N-1) ; 
		perturb(random_index); 
		double z_new = getZ(); 
		double dz = z_new - z; 

		if(z > target && dz > 0 ) dz = revert(random_index); 
		if(z < target && dz < 0 ) dz = revert(random_index); 

		if(dz!=0) z = z_new; 
	}

}


// monte carlo move and acceptance/rejection criteria 
bool mc_step(int w_index = numWindows-1) {

	double z_prev = getZ() ; 
	random_index = 1 + rand() % (N-1); // random int (1,N-1)  

	perturb(random_index); 

	double dE = deltaE(random_index, w_index, z_prev ); 

	// keeping RP, TP, RPTP constant here 
	// possible PROBLEM but maybe a necessary one, since sampling still happens. 
	/*
	if (diff(RP,getRP()) || diff(TP,getTP()) || diff(RPTP,getRPTP()) ) {
			revert(random_index) ; 
			return false; 
	
	}
	*/

	// Metropolis part 
	if(dE <= 0.) return true; 
	else if (rand()/double(RAND_MAX) > exp(-dE) ) {

		revert(random_index); 
		return false;
	}
	

	return true; 

}


double getE()  { 
	double E = 0. ; 
	for ( int i = 0; i+1 < N; i++) {
		E -= innerProduct(tx[i],ty[i],tz[i],tx[i+1],ty[i+1],tz[i+1]); 
	}
	return _ds*E ; 
}

double getBiasedE(int wi ) { 

	return getE() + V_bias(getZ(),wi) ; 
}

void writeZ(int step, int wi ) { 
	double bin_index = floor( getZ() * 1000) ; 

	zHists[wi][bin_index] += 1 ; 
	totalCounts[wi] += 1 ; 
}

void writeZFile(int step, int wi) {
	char zString[40];
	sprintf(zString,"%d\t%f\t%f\n",step, getZ() , getE()); 
	fputs(zString,windowFiles[wi]);

}

void WriteEventData(int step, int wi ) {

	writeZ(step,wi); 
	writeZFile(step,wi); 
}

/*
void reset() { 

	// must re-seed after a few billion steps 
	iseed = time(0); 
	srand(iseed);
	generator.seed(iseed);
	tx.assign(N,0); 
	ty.assign(N,0); 
	tz.assign(N,1); 

	if(getZ() < 1.0 ) {
		cout<< "can't normalize? "<< endl; 
		exit(1); 
	}

}
*/
void writeHistograms() {

	for(int j = 0; j < numWindows; j++ ) {
		double TC = totalCounts[j] ; 

		for(int i = 0; i < nbins; i++) {
			double binLowEdge = i/(double) nbins ; 
			double binCenter = binLowEdge + 1/(2.*nbins) ; 
			double binContent = zHists[j][i]; 
			double P = binContent / TC ; 

			char histVals[25]; 
			if(binContent != 0 ) {
				sprintf(histVals,"%f\t%f\n",binCenter, P ); 
				fputs(histVals,histFiles[j]) ;

			}

		}

	}
}

void write_metadata() {

	metaFile = fopen("metadata.dat","w") ; 

	for (int j = 0 ; j < numWindows; j++ ) {
		char data[100]; 
		sprintf(data, "/Users/paulglen/github/WLC/umbrellaSampling_harmonic/window_%d\t%f\t%f\t%d\t%f\t\n",j,zmin[j],K[j],0,ds) ; 
		fputs(data,metaFile) ; 
	}
	fclose(metaFile) ;

}

void checkNorms() { 
	// enforce normality of the t_i to within 1e-4 %  
	for(int i = 0; i < N; i++) {
		double norm = innerProduct(tx[i],ty[i],tz[i],tx[i],ty[i],tz[i]) ; 	
		if(fabs(norm - 1.0) > 1e-6) {
			cerr << " vector " << i << "failed to be normal " << std::endl; 
			cout << "instead has magnitude " << norm  << endl;
			exit(1); 
		}
	}
}

int cleanup() {
	fclose(zFile); 
	fclose(progressFile);
	return 0 ; 
}

