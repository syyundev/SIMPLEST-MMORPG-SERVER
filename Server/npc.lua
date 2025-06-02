myid = 99999;

function set_uid(x)
   myid = x;
end

function event_player_move(player)
   player_x = API_get_x(player);
   player_y = API_get_y(player);
   my_x = API_get_x(myid);
   my_y = API_get_y(myid);
   if (player_x == my_x) then
      if (player_y == my_y) then
         API_SendMessage(myid, player, "HELLO");
      end
   end
end

function random_move(monster, player)
    local move_count = API_get_monster_move_count(monster)
    if (move_count < 3) then
        API_MonsterRandomMove(monster)
    else
        API_SendMessage(monster, player, "BYE")
    end
end

