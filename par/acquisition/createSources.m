clearvars; close all;

OUTPUT_FILENAME='sources_test.txt';

%% Requiered Parameters
shotNo=[1,9];
X=[50,45]; % Coordinates in X (Grid points)
Y=[50,45]; % Coordinates in Y (Grid points) (depth)
Z=[50,45]; % Coordinates in Z (Grid points)
SOURCE_TYPE=[1,1]; % Source Type (1=P,2=vX,3=vY,4=vZ)
WAVELET_TYPE=[1,1]; % Wavelet Type (1=Synthetic)

%% Optional Parameters
WAVELET_SHAPE=[1,1]; % Wavelet Shape (1=Ricker,2=Sinw,3=sin^3,4=FGaussian,5=Spike/Delta,6=integral sin^3)
FC=[5,5]; % Center Frequency in Hz
AMP=[5,5]; % Amplitude
TShift=[0,0]; % Time shift in s


%% Write to file

% Create Matrix
SOURCE_FILE=[shotNo' X' Y' Z' SOURCE_TYPE' WAVELET_TYPE' WAVELET_SHAPE' FC' AMP' TShift'];

% Create header
headerline=["#shot_number","source_coordinate_(x)","source_coordinate_(y)",...
    "source_coordinate_(z)","source_type","wavelet_type","wavelet_shape",...
    "center_frequency","amplitude","time_shift"];

% Write txt file
fileID = fopen(OUTPUT_FILENAME,'w');
fprintf(fileID,'%s %s %s %s %s %s %s %s %s %s\n',headerline);
fprintf(fileID,'%i %i %i %i %i %i %i %g %g %g\n',SOURCE_FILE');
fclose(fileID);
%dlmwrite(OUTPUT_FILENAME,SOURCE_FILE,'delimiter',' ')
% writematrix(SOURCE_FILE,OUTPUT_FILENAME,'Delimiter','space') % introduced
% in Matlab 2019