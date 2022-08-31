#include<iostream>
#include"goldchase.h"
#include<unistd.h>
#include"goldchase.h"
#include<memory>
#include<fcntl.h>
#include<sys/mman.h>
#include<sys/stat.h>
#include<semaphore.h>
#include "Map.h"
#include "Game.h"

using namespace std;


int main(int argc,char* argv[])
{
	unique_ptr<Game> game;
	if(argc  == 1 ||argc  == 2)
	{
		if(argc  == 2)
		{
			
			game = make_unique<Game>(argv[1]);
			
		}
		else if(argc  == 1)
		{
			game = make_unique<Game>();
		}
		
		game->gameloop();
	}
	else
	{
		cerr<<"incorrect number of arguments"<<endl;
	} 
	
	return 0;
	
}
