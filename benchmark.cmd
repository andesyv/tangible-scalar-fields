echo pdbid,atomcount,^
sphereavgla,spheremaxla,sphereminla,spawnavgla,spawnmaxla,spawnminla,surfaceavgla,surfacemaxla,surfaceminla,shadeavgla,shademaxla,shademinla,fpsavgla,fpsminla,fpsmaxla,^
sphereavglb,spheremaxlb,sphereminlb,spawnavglb,spawnmaxlb,spawnminlb,surfaceavglb,surfacemaxlb,surfaceminlb,shadeavglb,shademaxlb,shademinlb,fpsavglb,fpsminlb,fpsmaxlb,^
sphereavglc,spheremaxlc,sphereminlc,spawnavglc,spawnmaxlc,spawnminlc,surfaceavglc,surfacemaxlc,surfaceminlc,shadeavglc,shademaxlc,shademinlc,fpsavglc,fpsminlc,fpsmaxlc,^
sphereavgld,spheremaxld,sphereminld,spawnavgld,spawnmaxld,spawnminld,surfaceavgld,surfacemaxld,surfaceminld,shadeavgld,shademaxld,shademinld,fpsavgld,fpsminld,fpsmaxld,^
sphereavgma,spheremaxma,sphereminma,spawnavgma,spawnmaxma,spawnminma,surfaceavgma,surfacemaxma,surfaceminma,shadeavgma,shademaxma,shademinma,fpsavgma,fpsminma,fpsmaxma,^
sphereavgmb,spheremaxmb,sphereminmb,spawnavgmb,spawnmaxmb,spawnminmb,surfaceavgmb,surfacemaxmb,surfaceminmb,shadeavgmb,shademaxmb,shademinmb,fpsavgmb,fpsminmb,fpsmaxmb,^
sphereavgmc,spheremaxmc,sphereminmc,spawnavgmc,spawnmaxmc,spawnminmc,surfaceavgmc,surfacemaxmc,surfaceminmc,shadeavgmc,shademaxmc,shademinmc,fpsavgmc,fpsminmc,fpsmaxmc,^
sphereavgmd,spheremaxmd,sphereminmd,spawnavgmd,spawnmaxmd,spawnminmd,surfaceavgmd,surfacemaxmd,surfaceminmd,shadeavgmd,shademaxmd,shademinmd,fpsavgmd,fpsminmd,fpsmaxmd,^
sphereavgha,spheremaxha,sphereminha,spawnavgha,spawnmaxha,spawnminha,surfaceavgha,surfacemaxha,surfaceminha,shadeavgha,shademaxha,shademinha,fpsavgha,fpsminha,fpsmaxha,^
sphereavghb,spheremaxhb,sphereminhb,spawnavghb,spawnmaxhb,spawnminhb,surfaceavghb,surfacemaxhb,surfaceminhb,shadeavghb,shademaxhb,shademinhb,fpsavghb,fpsminhb,fpsmaxhb,^
sphereavghc,spheremaxhc,sphereminhc,spawnavghc,spawnmaxhc,spawnminhc,surfaceavghc,surfacemaxhc,surfaceminhc,shadeavghc,shademaxhc,shademinhc,fpsavghc,fpsminhc,fpsmaxhc,^
sphereavghd,spheremaxhd,sphereminhd,spawnavghd,spawnmaxhd,spawnminhd,surfaceavghd,surfacemaxhd,surfaceminhd,shadeavghd,shademaxhd,shademinhd,fpsavghd,fpsminhd,fpsmaxhd > benchmark.csv

.\bin\Release\molumes .\dat\6B0X.pdb >> benchmark.csv
.\bin\Release\molumes .\dat\5VOX.pdb >> benchmark.csv
.\bin\Release\molumes .\dat\1AON.pdb >> benchmark.csv
.\bin\Release\molumes .\dat\5OT7.pdb >> benchmark.csv
.\bin\Release\molumes .\dat\4V6X.pdb >> benchmark.csv
.\bin\Release\molumes .\dat\4V4G.pdb >> benchmark.csv
.\bin\Release\molumes .\dat\5Y6P.pdb >> benchmark.csv
.\bin\Release\molumes .\dat\3J3Q.pdb >> benchmark.csv

