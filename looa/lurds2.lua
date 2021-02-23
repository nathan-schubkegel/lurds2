local function initState()

  local nextPlayerNumber = 0

  local function createPlayer(name)
    nextPlayerNumber = nextPlayerNumber + 1
    return { name = name, score = 0, playerNumber = nextPlayerNumber }
  end
  
  local function addPlayer(name)
    local player = createPlayer(name)
    t.playersByNumber[player.playerNumber] = player
    t.playersByName[player.name] = player
  end
  
  local t = {
    playersByNumber = {},
    playersByName = {}
  }
  
  addPlayer('Lord Durp')
  addPlayer('Stink-o-man')
  addPlayer('Danboat')

  return t
end

local function render(g, state)
  g.fillBlack()
  local y = 10
  local x = 10
  for playerNumber, player in pairs(state.playersByNumber) do
    g.drawText(x, y, player.score .. ' ' .. player.name)
  end
end

-- make a function for ticking the state

-- make list for queued mutations to be applied on the next tick

-- make a function for determining whether a player can validly queue a mutation

-- TODO: how to associate network connections with players? (future problem? yeah probly)

