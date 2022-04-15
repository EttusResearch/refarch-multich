%%
%% Copyright 2021-2022 Ettus Research, a National Instruments Brand
%%
%% SPDX-License-Identifier: GPL-3.0-or-later
%%
%% Pipe Example of how to interface with MRFRA %%
%{
    To use this example you must run the Arch_pip_example concurrently with
    this application. You must also specify a number of parameters the same
    for both this file and the Arch_pipe_example's configuration file. These
    parameters for this file is the following
        -numberOfFiles 
            The number of channels specified in the configuration file
        -folderName
            The absolute folder location that matches the absolute folder
            location specified in the configuration file.
        -numberOfSamples
            The number of samples to aquire in a burst. This is passed to
            the Arch_pipe_example
        -updateGraphNumber
            The number of iterations to do. Will loop through and aquire
            the numberOfSamples every iteration.
        -PipeFileBufferSize
            This selection must always be smaller than the
            PipeFileBufferSize that is specified in the configuration file.
            This number is in samples. The PipeFileBufferSize is in Bytes.
            1 IQ Sample = 4 Bytes

    The basic loop of operations is as follows.
        -Arch_pipe_example creates the Pipe files
        -Matlab waits until all files are created
        **Where the loop starts**
        -Matlab opens and writes the number of samples it's requested
        -Arch_pipe_example reads the number of samples requested
        -Matlab and Arch_pipe_example close the file
        -Arch_pipe_example writes the recieved data from the USRP
        -Matlab read the number of samples requested
        -Matlab and Arch_pipe_example close the file
        -Matlab displays the data.
        -Arch_pipe_example waits for the number of samples requested
        **This loop keeps running until requested number of samples is 0**
        -The number of samples is 0 and Arch_pipe_example shuts down
%}

function pipeMatlab()
    tStart = tic;
    % Parameters that must be configured correctly per your setup.
    folderName= '/home/cogrfserver/jovian_workarea/Testc++/fifo';
    numberOfFiles=32;
    numberOfSamples=int32(1000000);
    updateGraphNumber = 200;
    PipeFileBufferSize = 500000;
    
    %Delete the previous background pool just in case its still active
    delete(gcp('nocreate'))
    pool=parpool("local", numberOfFiles);
    tic
    data = distributed(complex(zeros(numberOfFiles,numberOfSamples, 'int16'),0));
    disp("created distributed data")
    toc
    %This loop is used to probe and check to see if the pipe example has
    %created the files that we are trying to read.
    loopTry=300;
    for index = 1:loopTry
         matfiles = dir(fullfile(folderName, '*.fifo'));
         if size(matfiles,1) ~= numberOfFiles
            pause(1);
         else
             break
         end
    end
    
    %% The Main loop %%
    %   This loop will update the graph the number of times specified
    %   The order of this loop is critical to have proper interface with
    %   the Pipe example
    for aquireData = 1:updateGraphNumber
        Iteration = tic;
        % Spawn background processes to write the number of samples we'd
        % like to read per channel
        backgroundOpen = cell(1,numberOfFiles);
        for index = 1:size(matfiles,1)
            backgroundOpen{index} = parfeval(@writedatatofile,1,matfiles(index),numberOfSamples);
        end
        % Read the number of bytes written to the pipe. This checks to make
        % sure that we wrote the data.
        iterationsToTry = 300;
        numberWritten = cell(1,numberOfFiles);
        for index = 1:size(matfiles,1)
            numberWritten{index} = getDataFromBackground(backgroundOpen{index},iterationsToTry);
            if size(numberWritten{index},1) == 0
                iterationsToTry =0;
            end
        end
        % Spawns background processes to read the number of samples we
        % specified.     
        tic
        spmd
            data(labindex,:)=readdatafromfile(matfiles(labindex),numberOfSamples, PipeFileBufferSize);
        end
        toc

        % Displays some information about the data
        disp("data")
        tic
        disp(size(data))
        toc
        tic
        for index = 1:size(data,1)
            disp(size(data(index,:),2))
        end
        toc
        tic
        Data_array = gather(data);
        disp("gathering Data")
        toc
        tic
        disp(size(Data_array))
        toc
        % Graph the first 4 channels
        %{
        for index = 1:4
            x = 0:1:size(imag(data{index}),2)-1;
            plot(x,real(data{index}),'DisplayName',['real' num2str(index)])
            hold on;
            plot(x,imag(data{index}),'DisplayName',['Imag' num2str(index)])
        end
        legend
        hold off;
        %}
        disp("iteration time is")
        toc(Iteration)
        if iterationsToTry == 0
            break
        end
    end
    %% End of the main loop Shutting down %%

    % Send a request for 0 samples
    for index = 1:size(matfiles,1)
        backgroundOpen{index} = parfeval(@writedatatofile,1,matfiles(index),int32(0));
    end
    % make sure we wrote that number of samples
    for index = 1:size(matfiles,1)
        numberWritten{index} = getDataFromBackground(backgroundOpen{index},20);
    end
    % Delete the background pool
    delete(pool)
    disp("end")
end

%% Process to return the results of the background process %%
function output = getDataFromBackground(process, loopTry)
    for index = 1:loopTry
        if contains(process.State,'finished')
           break
        end
        pause(.5); % an arbitrary wait time.
    end
    if contains(process.State,'finished') % The process finished
        output = fetchOutputs(process);
    else
        % This can happen if too many samples drop or the delay is too
        % short for the number of samples aquired
        disp("Failed to return data")
        cancel(process)
        output = [];
    end
end

%% A background process to read data from the pipe %%
function output = readdatafromfile(file, numberOfSamples, PipeFileBufferSize)
    loopTry=20; % Try to open the file. Should be fast
    for index = 1:loopTry
        [fileID] = fopen(append(file.folder,"/",file.name),'r');
        if fileID < 0
            pause(.1); % Arbitrary wait. Can be any time.
        else
            break;
        end
    end
    % If we are unable to open the file return zeros
    if (fileID <0) 
        output = [];
        return
    end

    numberOfSamplesLeft = numberOfSamples; % used as a countdown
    output = complex(zeros(1,numberOfSamples),0); % prebuild array for speed
    sizeA = [2 PipeFileBufferSize]; % Number of samples to read before returning
    iterator = 1; % used to iterate and populate output's array
    while( numberOfSamplesLeft > 0 )
        numberOfSamplesLeft = numberOfSamplesLeft-PipeFileBufferSize;
        returnedSize=0;
        % This is where it can get stuck because of too many dropped
        % samples.
        while (returnedSize==0) 
            A1 = fread(fileID,sizeA,'int16=>int16');
            returnedSize = size(A1,2);
        end
        output(iterator:(iterator+returnedSize-1)) = complex(A1(1,:),A1(2,:));
        iterator= iterator+returnedSize;
    end
    fclose(fileID);
end

%% A background process to write data from the pipe %%
function output = writedatatofile(file, data)
    loopTry=20;
    for index = 1:loopTry
        [fileID] = fopen(append(file.folder,"/",file.name),'w');
        if fileID < 0
            pause(.1); %Arbitrary wait to open the file. Should be fast
        else
            break;
        end
    end
    output = fwrite(fileID,data,"int32");
    fclose(fileID);
end
