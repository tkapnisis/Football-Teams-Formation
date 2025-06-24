%clc
clear all

run('\\fs09-rd\drgnbird\MVO\scoresheet_S3.m')

rating = rating0;

for i=1:size(score,1) % loopje van het aantal wedstrijden van het seizoen

        data(i,:) = zeros(1,size(rating,2)+2);
        rating(i+1,:) = rating(i,:);
        
        Rank1 = sum(rating(i,:).*Team1(i,:));
        Rank2 = sum(rating(i,:).*Team2(i,:));
        Rank_diff = (Rank1 - Rank2) / (sum(Team1(i,:))+sum(Team2(i,:))); % 1 ratingpunt verschil per speler = 1 doelpunt
        % Voorbeeld 1: een gelijkspel levert het zwakkere team punten op, het sterkere team verliest dan wat.
        % maw, bij een gelijkspel worden de ratings van de teams ook echt gelijk getrokken.
        % Voorbeeld 2: bij 5 tegen 5 en een ratingverschil van 10, verwacht het model het sterkere team met +1 wint.
        % Als dat ook gebeurt veranderen de ratings niet.
        
        % Eerst wordt de onglijkheid van de teams verrekend.
        rating(i+1,:) = rating(i+1,:) - Team1(i,:)*Rank_diff;
        rating(i+1,:) = rating(i+1,:) + Team2(i,:)*Rank_diff;
        
        if score(i,1)-score(i,2)>0 % winnaars krijgen index 1
            data(i,3:end) = data(i,3:end) + Team1(i,:)*1;
            data(i,3:end) = data(i,3:end) + Team2(i,:)*2;
        elseif score(i,1)-score(i,2)<0 % verliezers krijgen index 2
            data(i,3:end) = data(i,3:end) + Team1(i,:)*2;
            data(i,3:end) = data(i,3:end) + Team2(i,:)*1;
        else % gelijkspelers krijgen index 3
            data(i,3:end) = data(i,3:end) + Team1(i,:)*3;
            data(i,3:end) = data(i,3:end) + Team2(i,:)*3;
        end
        
        data(i,1:2) = sort(score(i,:),'descend');
        for j=3:size(data,2)
            if data(i,j) == 1 % winnaars krijgen ratingpunten erbij
                rating(i+1,j-2) = rating(i+1,j-2) + (data(i,1)-data(i,2));
            elseif data(i,j) == 2 % verliezers verliezen ratingpunten
                rating(i+1,j-2) = rating(i+1,j-2) - (data(i,1)-data(i,2));
            end
        end
        % bankzitter is altijd gemiddelde van de echte spelers
        rating(i+1,end) = mean(rating(i+1,1:end-3));
end

% Sorteren en printen van de nieuwe ratings
[ranks,n] = sort(rating(end,1:end-3)','descend'); % zonder XX34 XX45 en BANK in de ranking en grafiek
[ranks,n] = sort(rating(end,:)','descend'); % zonder XX34 XX45 3n BANK in de ranking en grafiek
ranking = names(n,:);
ranking = [ranking num2str(ranks)]

figure(666)
shg
for p = 1:length(n)
    if p<6
        %plot(100*((rating(:,n(p))-rating0(n(p)))/rating0(n(p))),'-x')
        plot(0:size(rating,1)-1,rating(:,n(p))-rating0(n(p)),'-x')
    elseif p<11
        %plot(100*((rating(:,n(p))-rating0(n(p)))/rating0(n(p))),'-^')
        plot(0:size(rating,1)-1,rating(:,n(p))-rating0(n(p)),'-^')
    elseif p<15
        %plot(100*((rating(:,n(p))-rating0(n(p)))/rating0(n(p))),'-o')
        plot(0:size(rating,1)-1,rating(:,n(p))-rating0(n(p)),'-o')
    else
        %plot(100*((rating(:,n(p))-rating0(n(p)))/rating0(n(p))),'-*')
        plot(0:size(rating,1)-1,rating(:,n(p))-rating0(n(p)),'-*')
    end
    hold on
end
hold off
legend(names(n,:),'Location','BestOutside')
title('Ratingverloop MVO-ers Seizoen 2018-2019')
xlabel('Wedstrijd #')
ylabel('Rating toename')
grid on

%% Nieuwe teams bepalen

% Vul in wie er komen spelen. Bij oneven aantal: selecteer XX34, XX45 of BANK erbij.
%         [NICK UGOK  RPEL TRNE LAHO TCL  JLU  JPB CVAC THUM  BVDM JRBR  XX34 XX45 BANK]
opkomst = [  0    0    1    0    1    1    0    1    0    1     1    1     1    0    0 ];

[a,spelers] = find(opkomst);
ns = length(spelers);
N = 10^(ns/2);
TeamAr = [];
TeamBr = [];
diff0 = 10000000;

%% Monte Carlo
% Beetje lelijke manier om alle combinaties te vinden. Mooier is commando
% "nchoosek" te gebruiken
for i=1:N % draai het loopje heel vaak
    k = randperm(ns); % maak twee willekeurige teams
    TeamAr = spelers(k(1:floor(length(k)/2)));
    TeamBr = spelers(k(floor(length(k)/2)+1:end));
    RankAr = sum(rating(end,TeamAr));
    RankBr = sum(rating(end,TeamBr));
    diffs = abs(RankAr-RankBr); % wat is het ratingverschil?
    if diffs < diff0 % als het ratingverschil van het gevonden team lager is dan de laagste tot nu toe
        MC_check = 0; 
        TeamA = TeamAr;
        TeamB = TeamBr;
        diff0 = diffs;
        RankA = RankAr;
        RankB = RankBr;
    elseif diffs == diff0;
        MC_check = MC_check+1; % Hoe vaak heeft ie uiteindelijk deze combinatie gevonden zonder een betere te vinden?
    end
end

disp('Monte Carlo geeft de volgende optimale teamindeling:')
disp(' ')
if RankA>RankB % Het op papier beste team krijgt altijd wit
    disp('Team WIT')
else
    disp('Team KLEUR')
end
disp([names(TeamA,:)]);% num2str(rating(end,TeamA)')];
disp(RankA)

if RankA>RankB
    disp('Team KLEUR')
else
    disp('Team WIT')
end
disp([names(TeamB,:)]);% num2str(rating(end,TeamB)')];
disp(RankB)

goals = sum(mean(score,1)); % predicted goals
Predicted_score = round([(goals+(RankA-RankB)/ns)/2 (goals-(RankA-RankB)/ns)/2],0)
