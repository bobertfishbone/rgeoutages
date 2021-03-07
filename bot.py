import os
from discord.ext import commands
import requests
import json
from dotenv import load_dotenv

load_dotenv()
TOKEN = os.getenv('DISCORD_TOKEN')
season = "2020"
bot = commands.Bot(command_prefix='$')

playerList = []

def refreshPlayers():
    response = requests.get(f'http://api.snooker.org/?t=10&st=p&s={season}')
    if response.ok:
        players = response.json()
        for player in players:
            playerList.append(player)
        print(playerList)
        return "Player list refreshed!"
    else:
        return "Cannot refresh player list."


def getMatches():
    response = requests.get('http://api.snooker.org/?t=7')
    if response.ok:
        return response.json()
    else:
        return "0"

def getPlayer(playerID):
    for player in playerList:
        if player['ID'] == playerID:
            return player

def getPlayerID(LastName):
    for player in playerList:
        if player['LastName'].lower() == LastName.lower():
            return player['ID']

def getTopX(x):
    response = requests.get(f"http://api.snooker.org/?rt=MoneyRankings&s={season}")
    rankings = json.loads(response.text)
    topx = ""
    for player in rankings:
        if player['Position'] <= x:
            playerInfo = getPlayer(player['PlayerID'])
            topx += f"{player['Position']}. {playerInfo['FirstName']} {playerInfo['LastName']}: {player['Sum']}\n"
        else:
            return topx

def getRanking(name1, name2):
    foundPlayer = None
    for player in playerList:
        if player['LastName'].lower() == name1.lower():
            if player['FirstName'].lower() == name2.lower():
                foundPlayer = player
                break
        elif player['FirstName'].lower() == name1.lower():
            if player['LastName'].lower() == name2.lower():
                foundPlayer = player
                break
    if foundPlayer is not None:
        response = requests.get(f"http://api.snooker.org/?rt=MoneyRankings&s={season}")
        rankings = json.loads(response.text)
        playerRank = None
        for rank in rankings:
            if rank['PlayerID'] == player['ID']:
                playerRank = rank
                break
        oneYearResponse = requests.get(f"http://api.snooker.org/?rt=OneYearMoneyRankings&s={season}")
        oneYearRankings = json.loads(oneYearResponse.text)
        oneYearRank = None
        for rank in oneYearRankings:
            if rank['PlayerID'] == player['ID']:
                oneYearRank = rank
                break
        return f"{foundPlayer['FirstName']} {foundPlayer['LastName']}\'s Current Ranking: {playerRank['Position']} ({oneYearRank['Position']}), Points: {playerRank['Sum']} ({oneYearRank['Sum']})"
    else:
        return "Player not found!"

@bot.command(name='live', help='Responds with current scores of live matches via snooker.org')
async def live_match(ctx):

    matchjson = getMatches()
    if matchjson == "0":
        response = "Error fetching live matches!"
    else: 
        playerOne = {}
        playerTwo = {}
        response = ""
        for match in matchjson:
            playerOne = getPlayer(match['Player1ID'])
            playerTwo = getPlayer(match['Player2ID'])
        if playerOne == {}:
            response = "No live matches!"
        else:
            response += f"{playerOne['LastName']} - {playerTwo['LastName']} Score: {match['Score1']}:{match['Score2']}\n"
    await ctx.send(response)

@bot.command(name='id')
async def player_id(ctx, LastName):
    playerID = getPlayerID(LastName)
    await ctx.send(playerID)

@bot.command(name='top', help='Responds with top X players by money ranking for current season via snooker.org')
async def top(ctx, rank: int):
    listing = getTopX(rank)
    await ctx.send(listing)

@bot.command(name='ranking', help='Responds with player ranking for current season (and one year list) via snooker.org')
async def rank(ctx, name1, name2):
    playerRank = ""
    playerRank = getRanking(name1, name2)
    await ctx.send(playerRank)


refreshPlayers()
bot.run(TOKEN)