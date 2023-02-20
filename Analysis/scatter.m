
% CC = 'C:\Users\Riddhi\Desktop\IDETC_Matlab_data\C1';
function [fileList, totalDist, Stiffness, PosLeft, PosRight] = scatter(keyword)

tempfileList = dir(fullfile(pwd,keyword));
tempfileList = {tempfileList.name};
fileList = string(tempfileList);
fileList = fileList';
for i = 1:length(fileList)
    fileID = fopen(fullfile(pwd,fileList(i,1)));
    fgets(fileID);
    k = fgetl(fileID);
    temp = textscan(k,'%f');
    Stiffness(i,1) = temp;   
    fgets(fileID);
    while ~feof(fileID)
        styData = textscan(fileID, '%f %f %f %f %f %f %f %f');
        styData = cell2mat(styData);
    end
    PosLeft = styData(1:end,2);
    PosRight = styData(1:end,6);
    distLeft(i,1) = abs(PosLeft(end,1) - PosLeft(1,1));
    distRight(i,1) = abs(PosRight(end,1) - PosRight(1,1));
    totalDist(i,1) = distLeft(i,1) + distRight(i,1);
%     fclose(fileID);
end 
Stiffness = cell2mat(Stiffness);
% figure
% % for i = 1:length(Stiffness)    
% %     scatter(Stiffness(i,1), totalDist(i,1));
% %     hold on;
% % end
% % % hold on;
% scatter(Stiffness(:,3),totalDist(:,1));




