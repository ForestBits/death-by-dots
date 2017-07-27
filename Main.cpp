#include <SFML/Graphics.hpp>
#include <vector>
#include <cassert>
#include <chrono>
#include <thread>
#include <random>
#include <iostream>
#include <cmath>
#include <mutex>
#include <future>

class BlockGrid;

float pointDirection(sf::Vector2f looker, sf::Vector2f target);
template <typename T> int sign(T val);
bool lineOfSight(sf::Vector2f point1, sf::Vector2f point2, BlockGrid & grid);
float distance(sf::Vector2f point1, sf::Vector2f point2);

class Screen
{
public:
	virtual Screen * update(sf::RenderWindow & window) = 0;
	virtual void draw(sf::RenderTarget & target) = 0;
};

class Random
{
	std::default_random_engine engine;

public:
	Random()
	{
		std::random_device device;

		engine.seed(device());
	}

	Random(long seed)
	{
		engine.seed(seed);
	}

	bool nextBool()
	{
		std::uniform_int_distribution<int> distribution(0, 1);

		return distribution(engine) != 0;
	}

	int nextInt()
	{
		return engine();
	}

	int nextInt(int n)
	{
		std::uniform_int_distribution<int> distribution(0, n - 1);

		return distribution(engine);
	}


	float nextFloat()
	{
		std::uniform_real_distribution<float> distribution(0.f, 1.f);

		return distribution(engine);
	}

	double nextDouble()
	{
		std::uniform_real_distribution<double> distribution(0.f, 1.f);

		return distribution(engine);
	}

	void setSeed(long seed)
	{
		engine.seed(seed);
	}

	int random_range(int lowerBound, int upperBound)
	{
		return nextInt(upperBound) + lowerBound;//TODO: fix this
	}

	int irandom_range(int lowerBound, int upperBound)
	{
		int number = nextInt((upperBound + 1) - lowerBound) + lowerBound;

		return number;
	}
};

class BlockGrid
{
	friend void generate(BlockGrid & grid);

	int blockSize;

	std::vector<std::vector<bool>> blocks;

	sf::Vector2i size;

	void setSolid(int x, int y, bool solid)
	{
		assert(x >= 0 && x < size.x && y >= 0 && y < size.y);

		blocks[x][y] = solid;
	}

public:
	BlockGrid(sf::Vector2i size) : blockSize(25), size(size)
	{
		blocks.resize(size.x);

		for (auto & vector : blocks)
			vector.resize(size.y);

		for (int x = 0; x < size.x; ++x)
			for (int y = 0; y < size.y; ++y)
				blocks[x][y] = false;
	}

	bool isSolid(int x, int y)
	{
		assert(x >= 0 && x < size.x && y >= 0 && y < size.y);

		return blocks[x][y];
	}

	sf::Vector2i getSize() {return size;}

	int getBlockSize() {return blockSize;}

	void draw(sf::RenderTarget & target, sf::FloatRect bounds)
	{
		static sf::RectangleShape rectangle(sf::Vector2f(blockSize, blockSize));

		int leftBlockBound = bounds.left/blockSize;
		int rightBlockBound = (bounds.left + bounds.width)/blockSize;
		int topBlockBound = bounds.top/blockSize;
		int bottomBlockBound = (bounds.top + bounds.height)/blockSize;

		if (leftBlockBound < 0)
			leftBlockBound = 0;

		if (topBlockBound < 0)
			topBlockBound = 0;

		if (rightBlockBound > size.x)
			rightBlockBound = size.x;

		if (bottomBlockBound > size.y)
			bottomBlockBound = size.y;

		for (int x = leftBlockBound; x < rightBlockBound; ++x)
			for (int y = topBlockBound; y < bottomBlockBound; ++y)
			{
				if (blocks[x][y])
					rectangle.setFillColor(sf::Color::Black);
				else
					rectangle.setFillColor(sf::Color::White);

				rectangle.setPosition(x*blockSize, y*blockSize);

				target.draw(rectangle);
			}
	}

	BlockGrid getSubset(sf::FloatRect bounds)
	{
		int leftBlockBound = bounds.left/blockSize;
		int rightBlockBound = (bounds.left + bounds.width)/blockSize;
		int topBlockBound = bounds.top/blockSize;
		int bottomBlockBound = (bounds.top + bounds.height)/blockSize;

		if (leftBlockBound < 0)
			leftBlockBound = 0;

		if (topBlockBound < 0)
			topBlockBound = 0;

		if (rightBlockBound > size.x)
			rightBlockBound = size.x;

		if (bottomBlockBound > size.y)
			bottomBlockBound = size.y;

		BlockGrid blockGrid(sf::Vector2i(rightBlockBound - leftBlockBound, bottomBlockBound - topBlockBound));

		for (int x = leftBlockBound; x < rightBlockBound; ++x)
			for (int y = topBlockBound; y < bottomBlockBound; ++y)
				blockGrid.setSolid(x - leftBlockBound, y - topBlockBound, blocks[x][y]);

		return blockGrid;
	}
};

void generate(BlockGrid & grid)
{
	Random random;

	for (int x = 3; x < grid.getSize().x - 4; ++x)
		for (int y = 0; y < grid.getSize().y; ++y)
			if (random.nextBool() && random.nextBool() && random.nextBool())
				grid.setSolid(x, y, true);
}

class Bullet
{
	float speed;
	float direction;

	int size;

	sf::Vector2f position;

public:
	Bullet(sf::Vector2f position, int size, float speed, float direction) : position(position), size(size), speed(speed), direction(direction) {}

	void update()
	{
		position.x += std::cos(direction)*speed;
		position.y += std::sin(direction)*speed;
	}

	void draw(sf::RenderTarget & target)
	{
		static sf::RectangleShape rectangle(sf::Vector2f(size, size));

		rectangle.setFillColor(sf::Color::Black);
		rectangle.setOrigin(size/2.f, size/2.f);
		rectangle.setPosition(position);

		target.draw(rectangle);
	}

	sf::Vector2f getPosition() {return position;}

	int getSize() {return size;}

	bool operator==(const Bullet & other) {return (position == other.position && speed == other.speed && direction == other.direction);}
};

class Turret
{
	int size;

	bool canShoot;

	sf::Vector2f position;
	
	int shotsPerSecond;

	sf::Clock clock;

	void shoot(sf::Vector2f target, std::vector<Bullet> & bullets)
	{
		float angle = pointDirection(position, target);

		bullets.push_back(Bullet(position, 10, 5, angle));
	}

public:
	Turret(sf::Vector2f position, int shotsPerSecond) : size(15), canShoot(false), position(position), shotsPerSecond(shotsPerSecond) {}

	void update(sf::Vector2f target, std::vector<Bullet> & bullets, BlockGrid & grid)
	{
		if (target == sf::Vector2f(0, 0))
			return;

		if (canShoot && lineOfSight(position, target, grid))
		{
			canShoot = false;

			clock.restart();

			shoot(target, bullets);
		}

		if (!canShoot && shotsPerSecond != 0)
		{
			if (clock.getElapsedTime().asSeconds() > 1/shotsPerSecond)
				canShoot = true;
		}
	}

	void draw(sf::RenderTarget & target)
	{
		static sf::RectangleShape rectangle(sf::Vector2f(size, size));

		rectangle.setFillColor(sf::Color(255, 255, 0));

		rectangle.setOrigin(rectangle.getSize().x/2, rectangle.getSize().y/2);

		rectangle.setPosition(position);

		target.draw(rectangle);
	}

	sf::Vector2f getPosition() {return position;}

	int getSize() {return size;}

	bool operator==(const Turret & other) {return position == other.position && shotsPerSecond == other.shotsPerSecond;}
};

class MovingTurret
{
	float speed;

	sf::Vector2f position;

	int size;
	int shotsPerSecond;
	
	bool canShoot;

	sf::Clock clock;

	void shoot(sf::Vector2f target, std::vector<Bullet> & bullets)
	{
		float angle = pointDirection(position, target);

		bullets.push_back(Bullet(position, 10, 5, angle));
	}

public:
	MovingTurret(sf::Vector2f position, float speed, int shotsPerSecond) : position(position), speed(speed), shotsPerSecond(shotsPerSecond), canShoot(false), size(10)
	{

	}

	void update(sf::Vector2f target, std::vector<Bullet> & bullets, BlockGrid & grid)
	{
		if (target == sf::Vector2f(0, 0))
			return;

		bool hasLineOfSight = lineOfSight(position, target, grid);

		if (canShoot && hasLineOfSight)
		{
			canShoot = false;

			clock.restart();

			shoot(target, bullets);
		}

		if (!canShoot && shotsPerSecond != 0)
		{
			if (clock.getElapsedTime().asSeconds() > 1/shotsPerSecond)
				canShoot = true;
		}

		if (!(distance(position, target) < 100 && hasLineOfSight))
		{
			sf::Vector2f cornerPosition;

			cornerPosition.x = position.x - size/2;
			cornerPosition.y = position.y - size/2;

			int xMove = target.x - position.x;
			int yMove = target.y - position.y;

			if (std::abs(xMove) > speed)
				xMove = xMove > 0 ? speed : -speed;

			if (std::abs(yMove) > speed)
				yMove = yMove > 0 ? speed : -speed;

			int prevX = position.x;
			int prevY = position.y;

			for (int i = 0; i < std::abs(xMove); ++i)
			{
				int increment = sign(xMove);

				position.x += increment;

				std::vector<sf::Vector2i> blocks;

				blocks.push_back(sf::Vector2i((position.x - size/2)/grid.getBlockSize(), (position.y - size/2)/grid.getBlockSize()));
				blocks.push_back(sf::Vector2i(std::ceil((position.x + size/2) /grid.getBlockSize()), std::ceil((position.y + size/2)/grid.getBlockSize())));
				blocks.push_back(sf::Vector2i(std::ceil((position.x + size/2)/grid.getBlockSize()), (position.y - size/2)/grid.getBlockSize()));
				blocks.push_back(sf::Vector2i((position.x - size/2)/grid.getBlockSize(), std::ceil((position.y + size/2)/grid.getBlockSize())));

				if (position.x < 0 || position.x > grid.getBlockSize()*grid.getSize().x - size)
				{
					position.x = prevX;

					break;
				}

				for (auto block : blocks)
					if (block.x < grid.getSize().x && block.y < grid.getSize().y && block.x >= 0 && block.y >= 0 && grid.isSolid(block.x, block.y))
					{
						position.x = prevX;

						break;
					}

				prevX = position.x;
			}

			for (int i = 0; i < std::abs(yMove); ++i)
			{
				int increment = sign(yMove);

				position.y += increment;

				std::vector<sf::Vector2i> blocks;

				blocks.push_back(sf::Vector2i((position.x - size/2)/grid.getBlockSize(), (position.y - size/2)/grid.getBlockSize()));
				blocks.push_back(sf::Vector2i(std::ceil((position.x + size/2) /grid.getBlockSize()), std::ceil((position.y + size/2)/grid.getBlockSize())));
				blocks.push_back(sf::Vector2i(std::ceil((position.x + size/2)/grid.getBlockSize()), (position.y - size/2)/grid.getBlockSize()));
				blocks.push_back(sf::Vector2i((position.x - size/2)/grid.getBlockSize(), std::ceil((position.y + size/2)/grid.getBlockSize())));

				if (position.y < 0 || position.y > grid.getBlockSize()*grid.getSize().y - size)
				{
					position.y = prevY;

					break;
				}

				for (auto block : blocks)
					if (block.x < grid.getSize().x && block.y < grid.getSize().y && block.x >= 0 && block.y >= 0 && grid.isSolid(block.x, block.y))
					{
						position.y = prevY;

						break;
					}

				prevY = position.y;
			}

			/*position = cornerPosition;

			int extra = (size/2) % 2 == 0 ? 0 : 1;

			position.x += size/2 + extra;
			position.y += size/2 + extra;*/
		}
	}

	void draw(sf::RenderTarget & target)
	{
		static sf::RectangleShape rectangle(sf::Vector2f(size, size));

		rectangle.setFillColor(sf::Color::Green);

		rectangle.setSize(sf::Vector2f(size, size));

		rectangle.setPosition(position);

		rectangle.setOrigin(rectangle.getSize().x/2, rectangle.getSize().y/2);

		target.draw(rectangle);
	}

	sf::Vector2f getPosition() {return position;}

	int getSize() {return size;}
};

class MovingSpawningTurret
{
	float speed;
	float spawnTime;

	sf::Vector2f position;

	int size;
	int shotsPerSecond;
	
	bool canShoot;

	sf::Clock clock;
	sf::Clock spawnClock;

	void shoot(sf::Vector2f target, std::vector<Bullet> & bullets)
	{
		float angle = pointDirection(position, target);

		bullets.push_back(Bullet(position, 10, 5, angle));
	}

public:
	MovingSpawningTurret(sf::Vector2f position, float speed, float spawnTime, int shotsPerSecond) : position(position), speed(speed), spawnTime(spawnTime), shotsPerSecond(shotsPerSecond), canShoot(false), size(10)
	{

	}

	void update(sf::Vector2f target, std::vector<MovingSpawningTurret> & turrets, std::vector<Bullet> & bullets, BlockGrid & grid)
	{
		if (target == sf::Vector2f(0, 0))
			return;

		bool hasLineOfSight = lineOfSight(position, target, grid);

		if (canShoot && hasLineOfSight)
		{
			canShoot = false;

			clock.restart();

			shoot(target, bullets);
		}

		if (!canShoot && shotsPerSecond != 0)
		{
			if (clock.getElapsedTime().asSeconds() > 1/shotsPerSecond)
				canShoot = true;
		}

		if (spawnClock.getElapsedTime().asSeconds() > spawnTime)
		{
			spawnClock.restart();

			Random random;

			std::vector<sf::Vector2i> emptyBlocks;

			int leftBound = (position.x - 100)/grid.getBlockSize();
			int topBound = (position.y - 100)/grid.getBlockSize();
			int rightBound = (position.x + std::ceil(100))/grid.getBlockSize();
			int bottomBound = (position.y + std::ceil(100))/grid.getBlockSize();

			if (leftBound < 0)
				leftBound = 0;

			if (rightBound >= grid.getSize().x)
				rightBound = grid.getSize().x - 2;

			if (topBound < 0)
				topBound = 0;

			if (bottomBound >= grid.getSize().y)
				bottomBound = grid.getSize().y - 1;

			for (int x = leftBound; x < rightBound; ++x)
				for (int y = topBound; y < bottomBound; ++y)
					if (!grid.isSolid(x, y))
						emptyBlocks.push_back(sf::Vector2i(x, y));

			if (!emptyBlocks.empty())
			{
				int randomIndex = random.nextInt(emptyBlocks.size());

				sf::Vector2f position(emptyBlocks[randomIndex].x*grid.getBlockSize() + grid.getBlockSize()/2, emptyBlocks[randomIndex].y*grid.getBlockSize() + grid.getBlockSize()/2);

				turrets.push_back(MovingSpawningTurret(position, 1, 1, 1));
			}
		}

		if (!(distance(position, target) < 100 && hasLineOfSight))
		{
			sf::Vector2f cornerPosition;

			cornerPosition.x = position.x - size/2;
			cornerPosition.y = position.y - size/2;

			int xMove = target.x - position.x;
			int yMove = target.y - position.y;

			if (std::abs(xMove) > speed)
				xMove = xMove > 0 ? speed : -speed;

			if (std::abs(yMove) > speed)
				yMove = yMove > 0 ? speed : -speed;

			int prevX = position.x;
			int prevY = position.y;

			for (int i = 0; i < std::abs(xMove); ++i)
			{
				int increment = sign(xMove);

				position.x += increment;

				std::vector<sf::Vector2i> blocks;

				blocks.push_back(sf::Vector2i((position.x - size/2)/grid.getBlockSize(), (position.y - size/2)/grid.getBlockSize()));
				blocks.push_back(sf::Vector2i(std::ceil((position.x + size/2) /grid.getBlockSize()), std::ceil((position.y + size/2)/grid.getBlockSize())));
				blocks.push_back(sf::Vector2i(std::ceil((position.x + size/2)/grid.getBlockSize()), (position.y - size/2)/grid.getBlockSize()));
				blocks.push_back(sf::Vector2i((position.x - size/2)/grid.getBlockSize(), std::ceil((position.y + size/2)/grid.getBlockSize())));

				if (position.x < 0 || position.x > grid.getBlockSize()*grid.getSize().x - size)
				{
					position.x = prevX;

					break;
				}

				for (auto block : blocks)
					if (block.x < grid.getSize().x && block.y < grid.getSize().y && block.x >= 0 && block.y >= 0 && grid.isSolid(block.x, block.y))
					{
						position.x = prevX;

						break;
					}

				prevX = position.x;
			}

			for (int i = 0; i < std::abs(yMove); ++i)
			{
				int increment = sign(yMove);

				position.y += increment;

				std::vector<sf::Vector2i> blocks;

				blocks.push_back(sf::Vector2i((position.x - size/2)/grid.getBlockSize(), (position.y - size/2)/grid.getBlockSize()));
				blocks.push_back(sf::Vector2i(std::ceil((position.x + size/2) /grid.getBlockSize()), std::ceil((position.y + size/2)/grid.getBlockSize())));
				blocks.push_back(sf::Vector2i(std::ceil((position.x + size/2)/grid.getBlockSize()), (position.y - size/2)/grid.getBlockSize()));
				blocks.push_back(sf::Vector2i((position.x - size/2)/grid.getBlockSize(), std::ceil((position.y + size/2)/grid.getBlockSize())));

				if (position.y < 0 || position.y > grid.getBlockSize()*grid.getSize().y - size)
				{
					position.y = prevY;

					break;
				}

				for (auto block : blocks)
					if (block.x < grid.getSize().x && block.y < grid.getSize().y && block.x >= 0 && block.y >= 0 && grid.isSolid(block.x, block.y))
					{
						position.y = prevY;

						break;
					}

				prevY = position.y;
			}

			/*position = cornerPosition;

			int extra = (size/2) % 2 == 0 ? 0 : 1;

			position.x += size/2 + extra;
			position.y += size/2 + extra;*/
		}
	}

	void draw(sf::RenderTarget & target)
	{
		static sf::RectangleShape rectangle(sf::Vector2f(size, size));

		rectangle.setFillColor(sf::Color::Magenta);

		rectangle.setSize(sf::Vector2f(size, size));

		rectangle.setPosition(position);

		rectangle.setOrigin(rectangle.getSize().x/2, rectangle.getSize().y/2);

		target.draw(rectangle);
	}

	sf::Vector2f getPosition() {return position;}

	int getSize() {return size;}
};

class Player
{
	sf::Vector2f position;

	int size;

public:
	Player() : position(0, 0), size(25) {}

	void update(BlockGrid & grid)
	{
		int xMove = 0;
		int yMove = 0;

		int prevX = position.x;
		int prevY = position.y;

		int moveAmount = 3;

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) || sf::Keyboard::isKeyPressed(sf::Keyboard::A))
			xMove = -moveAmount;

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) || sf::Keyboard::isKeyPressed(sf::Keyboard::D))
			xMove = moveAmount;

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up) || sf::Keyboard::isKeyPressed(sf::Keyboard::W))
			yMove = -moveAmount;

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) || sf::Keyboard::isKeyPressed(sf::Keyboard::S))
			yMove = moveAmount;

		

		for (int i = 0; i < std::abs(xMove); ++i)
		{
			int increment = sign(xMove);

			position.x += increment;

			std::vector<sf::Vector2i> blocks;

			blocks.push_back(sf::Vector2i(position.x/grid.getBlockSize(), position.y/grid.getBlockSize()));
			blocks.push_back(sf::Vector2i(std::ceil(position.x/grid.getBlockSize()), std::ceil(position.y/grid.getBlockSize())));
			blocks.push_back(sf::Vector2i(std::ceil(position.x/grid.getBlockSize()), position.y/grid.getBlockSize()));
			blocks.push_back(sf::Vector2i(position.x/grid.getBlockSize(), std::ceil(position.y/grid.getBlockSize())));

			if (position.x < 0 || position.x > grid.getBlockSize()*grid.getSize().x - size)
			{
				position.x = prevX;

				break;
			}

			for (auto block : blocks)
				if (grid.isSolid(block.x, block.y))
				{
					position.x = prevX;

					break;
				}

			prevX = position.x;
		}

		for (int i = 0; i < std::abs(yMove); ++i)
		{
			int increment = sign(yMove);

			position.y += increment;

			std::vector<sf::Vector2i> blocks;

			blocks.push_back(sf::Vector2i(position.x/grid.getBlockSize(), position.y/grid.getBlockSize()));
			blocks.push_back(sf::Vector2i(std::ceil(position.x/grid.getBlockSize()), std::ceil(position.y/grid.getBlockSize())));
			blocks.push_back(sf::Vector2i(std::ceil(position.x/grid.getBlockSize()), position.y/grid.getBlockSize()));
			blocks.push_back(sf::Vector2i(position.x/grid.getBlockSize(), std::ceil(position.y/grid.getBlockSize())));

			if (position.y < 0 || position.y > grid.getBlockSize()*grid.getSize().y - size)
			{
				position.y = prevY;

				break;
			}

			for (auto block : blocks)
				if (grid.isSolid(block.x, block.y))
				{
					position.y = prevY;

					break;
				}

			prevY = position.y;
		}
	}

	void draw(sf::RenderTarget & target)
	{
		static sf::RectangleShape rectangle(sf::Vector2f(size, size));

		rectangle.setFillColor(sf::Color::Red);

		rectangle.setPosition(position);

		target.draw(rectangle);
	}

	int getSize() {return size;}

	sf::Vector2f getPosition() {return position;}
};

	

float pointDirection(sf::Vector2f looker, sf::Vector2f target)
{
	return 3.12 + std::atan2(looker.y - target.y, looker.x - target.x);
}

float distance(sf::Vector2f point1, sf::Vector2f point2)
{
	return std::sqrt(std::pow(point1.x - point2.x, 2) + std::pow(point1.y - point2.y, 2));
}

bool bulletGridCollision(Bullet bullet, BlockGrid & blockGrid, int blockSize)
{
	int leftBound = (bullet.getPosition().x - bullet.getSize()/2.f)/blockGrid.getBlockSize();
	int topBound = (bullet.getPosition().y - bullet.getSize()/2.f)/blockGrid.getBlockSize();
	int rightBound = (bullet.getPosition().x + std::ceil(bullet.getSize()/2.f))/blockGrid.getBlockSize();
	int bottomBound = (bullet.getPosition().y + std::ceil(bullet.getSize()/2.f))/blockGrid.getBlockSize();

	std::vector<sf::Vector2i> blocks;

	for (int x = leftBound; x < rightBound; ++x)
		for (int y = topBound; y < bottomBound; ++y)
			blocks.push_back(sf::Vector2i(x, y));

	for (auto block : blocks)
		if (blockGrid.isSolid(block.x, block.y))
			return true;

	return false;
	

	//return blockGrid.isSolid(static_cast<int> (bullet.getPosition().x/blockSize), static_cast<int> (bullet.getPosition().y/blockSize));
}

bool playerBulletCollision(Player player, Bullet bullet)
{
	sf::FloatRect playerRect;

	playerRect.left = player.getPosition().x;
	playerRect.top = player.getPosition().y;

	playerRect.width = player.getSize();
	playerRect.height = player.getSize();

	sf::FloatRect bulletRect;

	bulletRect.left = bullet.getPosition().x - bullet.getSize()/2.f;
	bulletRect.top = bullet.getPosition().y - bullet.getSize()/2.f;

	bulletRect.width = bullet.getSize();
	bulletRect.height = bullet.getSize();

	if (playerRect.intersects(bulletRect))
		return true;
	else
		return false;
}

template <typename T> int sign(T val) {
    return (T(0) < val) - (val < T(0));
}

void populateTurrets(std::vector<Turret> & turrets, BlockGrid & grid)
{
	std::vector<sf::Vector2i> emptyBlocks;

	for (int x = 3; x < grid.getSize().x - 4; ++x)
		for (int y = 0; y < grid.getSize().y; ++y)
			if (!grid.isSolid(x, y))
				emptyBlocks.push_back(sf::Vector2i(x, y));
		

	Random random;

	for (auto block : emptyBlocks)
		if (random.irandom_range(0, 64) == 0)
			turrets.push_back(Turret(sf::Vector2f(block.x*grid.getBlockSize() + grid.getBlockSize()/2, block.y*grid.getBlockSize() + grid.getBlockSize()/2), 1));
}

void populateMovingTurrets(std::vector<MovingTurret> & turrets, BlockGrid & grid)
{
	std::vector<sf::Vector2i> emptyBlocks;

	for (int x = 3; x < grid.getSize().x - 4; ++x)
		for (int y = 0; y < grid.getSize().y; ++y)
			if (!grid.isSolid(x, y))
				emptyBlocks.push_back(sf::Vector2i(x, y));
		

	Random random;

	for (auto block : emptyBlocks)
		if (random.irandom_range(0, 500) == 0)
			turrets.push_back(MovingTurret(sf::Vector2f(block.x*grid.getBlockSize() + grid.getBlockSize()/2, block.y*grid.getBlockSize() + grid.getBlockSize()/2), 1, 1));
}

void populateMovingSpawningTurrets(std::vector<MovingSpawningTurret> & turrets, BlockGrid & grid)
{
	std::vector<sf::Vector2i> emptyBlocks;

	for (int x = 3; x < grid.getSize().x - 4; ++x)
		for (int y = 0; y < grid.getSize().y; ++y)
			if (!grid.isSolid(x, y))
				emptyBlocks.push_back(sf::Vector2i(x, y));
		

	Random random;

	for (auto block : emptyBlocks)
		if (random.irandom_range(0, 500) == 0)
			turrets.push_back(MovingSpawningTurret(sf::Vector2f(block.x*grid.getBlockSize() + grid.getBlockSize()/2, block.y*grid.getBlockSize() + grid.getBlockSize()/2), 1, 1, 1));
}

bool lineOfSight(sf::Vector2f point1, sf::Vector2f point2, BlockGrid & grid)
{
	float angle = pointDirection(point1, point2);

	for (float distanceWalked = 0; distanceWalked < distance(point1, point2); distanceWalked += 0.9)
	{
		int posX = point1.x + std::cos(angle)*distanceWalked;
		int posY = point1.y + std::sin(angle)*distanceWalked;

		if (grid.isSolid(posX/grid.getBlockSize(), posY/grid.getBlockSize()))
			return false;
	}

	return true;
}

class MainMenuScreen : public Screen
{
	sf::Text titleText;
	sf::Text quitText;
	sf::Text playText;
	sf::Text instructionsText;

	sf::Font * font;
public:
	MainMenuScreen(sf::Vector2i windowSize, sf::Font & font);

	Screen * update(sf::RenderWindow & window);

	void draw(sf::RenderTarget & target);
};

class InstructionsScreen : public Screen
{
	sf::Text titleText;
	sf::Text backText;
	sf::Text playerText;
	sf::Text bulletText;
	sf::Text turretText;
	sf::Text movingTurretText;
	sf::Text safeZoneText;
	sf::Text finishZoneText;

	sf::RectangleShape playerRectangle;
	sf::RectangleShape bulletRectangle;
	sf::RectangleShape turretRectangle;
	sf::RectangleShape movingTurretRectangle;
	sf::RectangleShape safeZoneRectangle;
	sf::RectangleShape finishZoneRectangle;

	sf::Font * font;

	sf::Vector2f windowSize;
public:
	InstructionsScreen(sf::Vector2i windowSize, sf::Font & font);

	Screen * update(sf::RenderWindow & window);
	void draw(sf::RenderTarget & target);

};

class DeadScreen : public Screen
{
	sf::Text diedText;
	sf::Text explanationText;
	sf::Text backText;

	sf::Font * font;

	sf::Vector2i windowSize;

public:
	DeadScreen(sf::Vector2i windowSize, sf::Font & font);

	Screen * update(sf::RenderWindow & window);

	void draw(sf::RenderTarget & target);
};

class WinScreen : public Screen
{
	sf::Text winText;
	sf::Text explanationText;
	sf::Text backText;

	sf::Font * font;

	sf::Vector2i windowSize;

public:
	WinScreen(sf::Vector2i windowSize, sf::Font & font);

	Screen * update(sf::RenderWindow & window);

	void draw(sf::RenderTarget & target);
};

class GameScreen : public Screen
{
	std::vector<Bullet> bullets;
	std::vector<Turret> turrets;
	std::vector<MovingTurret> movingTurrets;
	//std::vector<MovingSpawningTurret> movingSpawningTurrets;

	std::vector<Turret *> activeTurrets;
	std::vector<MovingTurret *> activeMovingTurrets;
	//std::vector<MovingSpawningTurret *> activeMovingSpawningTurrets;

	Player player;

	BlockGrid blockGrid;

	sf::FloatRect activeBounds;

	sf::View view;

	std::vector<int> startRectanglePositions;
	std::vector<sf::RectangleShape> rectangles;
	sf::RectangleShape startRectangle;
	sf::RectangleShape endRectangle;

	sf::Font * font;

public:
	GameScreen(const sf::RenderWindow & window, sf::Font & font) : blockGrid(sf::Vector2i(100, window.getSize().y/20)), view(window.getDefaultView()), startRectangle(sf::Vector2f(blockGrid.getBlockSize()*3, blockGrid.getSize().y*blockGrid.getBlockSize())), endRectangle(sf::Vector2f(blockGrid.getBlockSize()*3, blockGrid.getSize().y*blockGrid.getBlockSize()))
	{
		this->font = &font;

		generate(blockGrid);

		populateTurrets(turrets, blockGrid);
		populateMovingTurrets(movingTurrets, blockGrid);
		//populateMovingSpawningTurrets(movingSpawningTurrets, blockGrid);

		activeBounds.left = 0;
		activeBounds.top = 0;
		activeBounds.width = window.getSize().x*1.1;
		activeBounds.height = window.getSize().y*1.1;

		for (int i = 0; i < 1; ++i)
			startRectanglePositions.push_back(i*600);

		startRectangle.setPosition(0, 0);

		startRectangle.setFillColor(sf::Color(0, 0, 255, 50));

		for (int i = 0; i < startRectanglePositions.size(); ++i)
		{
			sf::RectangleShape rectangle(startRectangle);

			rectangle.setPosition(startRectanglePositions[i], 0);

			rectangles.push_back(rectangle);
		}

		endRectangle.setPosition(blockGrid.getBlockSize()*(blockGrid.getSize().x - 3), 0);

		endRectangle.setFillColor(sf::Color(255, 0, 0, 50));
	}

	Screen * update(sf::RenderWindow & window)
	{
		sf::Event evt;

		while (window.pollEvent(evt))
		{
			if (evt.type == sf::Event::Closed)
				std::exit(0);
		}

		for (Bullet & bullet : bullets)
			bullet.update();

		activeTurrets.clear();
		activeMovingTurrets.clear();
//		activeMovingSpawningTurrets.clear();

		for (Turret & turret : turrets)
			if (turret.getPosition().x > activeBounds.left - 10 && turret.getPosition().x < activeBounds.left + activeBounds.width + 10 && turret.getPosition().y > activeBounds.top - 10 && turret.getPosition().y < activeBounds.top + activeBounds.height + 10)
				activeTurrets.push_back(&turret);

		for (MovingTurret & turret : movingTurrets)
			if (turret.getPosition().x > activeBounds.left - 10 && turret.getPosition().x < activeBounds.left + activeBounds.width + 10 && turret.getPosition().y > activeBounds.top - 10 && turret.getPosition().y < activeBounds.top + activeBounds.height + 10)
				activeMovingTurrets.push_back(&turret);

		/*for (MovingSpawningTurret & turret : movingSpawningTurrets)
			if (turret.getPosition().x > activeBounds.left - 10 && turret.getPosition().x < activeBounds.left + activeBounds.width + 10 && turret.getPosition().y > activeBounds.top - 10 && turret.getPosition().y < activeBounds.top + activeBounds.height + 10)
				activeMovingSpawningTurrets.push_back(&turret);*/

		if (sf::FloatRect(player.getPosition().x, player.getPosition().y, player.getSize(), player.getSize()).intersects(endRectangle.getGlobalBounds()))
		{
			window.setView(window.getDefaultView());

			return new WinScreen(sf::Vector2i(window.getSize().x, window.getSize().y), *font);
		}

		bool playerSafe = false;

		for (auto rect : rectangles)
			if (sf::FloatRect(player.getPosition().x, player.getPosition().y, player.getSize(), player.getSize()).intersects(rect.getGlobalBounds()))
				playerSafe = true;

		sf::Vector2f target = player.getPosition();

		if (playerSafe)
			target = sf::Vector2f(0, 0);

		for (Turret * turret : activeTurrets)
			turret->update(target, bullets, blockGrid);

		for (MovingTurret * turret : activeMovingTurrets)
			turret->update(target, bullets, blockGrid);

		/*for (MovingSpawningTurret * turret : activeMovingSpawningTurrets)
			turret->update(target, movingSpawningTurrets, bullets, blockGrid);*/

		player.update(blockGrid);

		activeBounds.left = view.getCenter().x - view.getSize().x/2;
		activeBounds.top = view.getCenter().y - view.getSize().y/2;

		std::vector<Bullet> toRemove;

		for (Bullet & bullet : bullets)
		{
			if (bullet.getPosition().x < 0 || bullet.getPosition().x >= blockGrid.getBlockSize()*blockGrid.getSize().x || bullet.getPosition().y < 0 || bullet.getPosition().y >= blockGrid.getBlockSize()*blockGrid.getSize().y)
			{
				toRemove.push_back(bullet);

				continue;
			}

			if (bulletGridCollision(bullet, blockGrid, 25))
				toRemove.push_back(bullet);
		}

		for (Bullet & bullet : toRemove)
			bullets.erase(std::find(bullets.begin(), bullets.end(), bullet));

		for (Bullet & bullet : bullets)
			if (!playerSafe && playerBulletCollision(player, bullet))
			{
				window.setView(window.getDefaultView());
				
				return new DeadScreen(sf::Vector2i(window.getSize().x, window.getSize().y), *font);
			}

		return this;
	}

	void draw(sf::RenderTarget & target)
	{
		target.clear(sf::Color::White);

		view.setCenter(player.getPosition());

		if (view.getCenter().x - view.getSize().x/2 < 0)
			view.setCenter(view.getSize().x/2, view.getCenter().y);

		if (view.getCenter().y - view.getSize().y/2 < 0)
			view.setCenter(view.getCenter().x, view.getSize().y/2);

		if (view.getCenter().x >= blockGrid.getBlockSize()*blockGrid.getSize().x - view.getSize().x/2)
			view.setCenter(blockGrid.getBlockSize()*blockGrid.getSize().x - view.getSize().x/2, view.getCenter().y);

		if (view.getCenter().y >= blockGrid.getBlockSize()*blockGrid.getSize().y - view.getSize().y/2)
			view.setCenter(view.getCenter().x, blockGrid.getBlockSize()*blockGrid.getSize().y - view.getSize().y/2);



		target.setView(view);

		blockGrid.draw(target, activeBounds);

		for (auto rect : rectangles)
			target.draw(rect);//target.draw(startRectangle);
		target.draw(endRectangle);

		player.draw(target);

		for (Bullet & bullet : bullets)
			bullet.draw(target);

		for (Turret * turret : activeTurrets)
			turret->draw(target);

		for (MovingTurret * turret : activeMovingTurrets)
			turret->draw(target);

/*		for (MovingSpawningTurret * turret : activeMovingSpawningTurrets)
			turret->draw(target);*/
	}
};

MainMenuScreen::MainMenuScreen(sf::Vector2i windowSize, sf::Font & font) : font(&font)
{
	titleText.setFont(font);
	quitText.setFont(font);
	playText.setFont(font);
	instructionsText.setFont(font);

	titleText.setFillColor(sf::Color::Black);
	titleText.setCharacterSize(30);
	titleText.setString("Death By Dots");
	titleText.setOrigin(titleText.getLocalBounds().width/2, titleText.getLocalBounds().height/2);
	titleText.setPosition(windowSize.x/2, windowSize.y/7);

	playText.setFillColor(sf::Color::Black);
	playText.setCharacterSize(20);
	playText.setString("Play");
	playText.setOrigin(playText.getLocalBounds().width/2, playText.getLocalBounds().height/2);
	playText.setPosition(windowSize.x/2, windowSize.y/4);

	instructionsText.setFillColor(sf::Color::Black);
	instructionsText.setCharacterSize(20);
	instructionsText.setString("Instructions");
	instructionsText.setOrigin(instructionsText.getLocalBounds().width/2, instructionsText.getLocalBounds().height/2);
	instructionsText.setPosition(windowSize.x/2, windowSize.y/3);

	quitText.setFillColor(sf::Color::Black);
	quitText.setCharacterSize(20);
	quitText.setString("Quit");
	quitText.setOrigin(quitText.getLocalBounds().width/2, quitText.getLocalBounds().height/2);
	quitText.setPosition(windowSize.x/2, windowSize.y/2);
}

Screen * MainMenuScreen::update(sf::RenderWindow & window)
{
	sf::Event evt;

	while (window.pollEvent(evt))
	{
		if (evt.type == sf::Event::Closed)
			return nullptr;

		if (evt.type == sf::Event::MouseButtonPressed)
		{
			if (evt.mouseButton.x >= playText.getGlobalBounds().left && evt.mouseButton.y >= playText.getGlobalBounds().top &&
				evt.mouseButton.x < playText.getGlobalBounds().left + playText.getGlobalBounds().width &&
				evt.mouseButton.y < playText.getGlobalBounds().top + playText.getGlobalBounds().height)
			{
					return new GameScreen(window, *font);
			}

			if (evt.mouseButton.x >= instructionsText.getGlobalBounds().left && evt.mouseButton.y >= instructionsText.getGlobalBounds().top &&
				evt.mouseButton.x < instructionsText.getGlobalBounds().left + instructionsText.getGlobalBounds().width &&
				evt.mouseButton.y < instructionsText.getGlobalBounds().top + instructionsText.getGlobalBounds().height)
			{
				return new InstructionsScreen(sf::Vector2i(window.getSize().x, window.getSize().y), *font);
			}

			if (evt.mouseButton.x >= quitText.getGlobalBounds().left && evt.mouseButton.y >= quitText.getGlobalBounds().top &&
				evt.mouseButton.x < quitText.getGlobalBounds().left + quitText.getGlobalBounds().width &&
				evt.mouseButton.y < quitText.getGlobalBounds().top + quitText.getGlobalBounds().height)
			{
				return nullptr;
			}
		}
	}

	return this;
}

void MainMenuScreen::draw(sf::RenderTarget & target)
{
	target.clear(sf::Color::White);

	target.draw(titleText);
	target.draw(playText);
	target.draw(instructionsText);
	target.draw(quitText);
}

InstructionsScreen::InstructionsScreen(sf::Vector2i windowSize, sf::Font & font)
{
	windowSize = windowSize;

	this->font = &font;

	titleText.setFont(font);
	backText.setFont(font);
	playerText.setFont(font);
	bulletText.setFont(font);
	turretText.setFont(font);
	movingTurretText.setFont(font);
	safeZoneText.setFont(font);
	finishZoneText.setFont(font);


	titleText.setString("Instructions");
	titleText.setCharacterSize(30);
	titleText.setOrigin(titleText.getLocalBounds().width/2, titleText.getLocalBounds().height/5);
	titleText.setPosition(windowSize.x/2, windowSize.y/7);
	titleText.setFillColor(sf::Color::Black);

	backText.setString("Main Menu");
	backText.setCharacterSize(20);
	backText.setOrigin(backText.getLocalBounds().width/2, backText.getLocalBounds().height/2);
	backText.setPosition(windowSize.x/2, windowSize.y/1.2);
	backText.setFillColor(sf::Color::Black);


	Player player;

	playerRectangle.setSize(sf::Vector2f(player.getSize(), player.getSize()));
	playerRectangle.setFillColor(sf::Color::Red);

	playerRectangle.setPosition(windowSize.x/8, windowSize.y/5);

	playerText.setString("This is you. Move with the arrow keys or A,W,S,D.");
	playerText.setCharacterSize(20);
	playerText.setPosition(playerRectangle.getPosition().x + playerRectangle.getGlobalBounds().width + 10, playerRectangle.getPosition().y);
	playerText.setFillColor(sf::Color::Black);

	Bullet bullet(sf::Vector2f(), 10, 10, 0);

	bulletRectangle.setSize(sf::Vector2f(bullet.getSize(), bullet.getSize()));
	bulletRectangle.setFillColor(sf::Color::Black);

	bulletRectangle.setPosition(windowSize.x/8, windowSize.y/4);

	bulletText.setString("This is a bullet. Don't get hit by them.");
	bulletText.setCharacterSize(20);
	bulletText.setPosition(bulletRectangle.getPosition().x + bulletRectangle.getGlobalBounds().width + 10, bulletRectangle.getPosition().y);
	bulletText.setFillColor(sf::Color::Black);

	Turret turret(sf::Vector2f(), 0);

	turretRectangle.setSize(sf::Vector2f(turret.getSize(), turret.getSize()));
	turretRectangle.setFillColor(sf::Color::Yellow);

	turretRectangle.setPosition(windowSize.x/8, windowSize.y/3.5);

	turretText.setString("This is a turret. It shoots bullets");
	turretText.setCharacterSize(20);
	turretText.setPosition(turretRectangle.getPosition().x + turretRectangle.getGlobalBounds().width + 10, turretRectangle.getPosition().y);
	turretText.setFillColor(sf::Color::Black);

	MovingTurret movingTurret(sf::Vector2f(), 0, 0);

	movingTurretRectangle.setSize(sf::Vector2f(movingTurret.getSize(), movingTurret.getSize()));
	movingTurretRectangle.setFillColor(sf::Color::Green);

	movingTurretRectangle.setPosition(windowSize.x/8, windowSize.y/3.f);

	movingTurretText.setString("Like a turret, but it moves.");
	movingTurretText.setCharacterSize(20);
	movingTurretText.setPosition(movingTurretRectangle.getPosition().x + movingTurretRectangle.getGlobalBounds().width + 10, movingTurretRectangle.getPosition().y);
	movingTurretText.setFillColor(sf::Color::Black);

	BlockGrid blockGrid(sf::Vector2i(0, 0));

	safeZoneRectangle.setSize(sf::Vector2f(blockGrid.getBlockSize(), blockGrid.getBlockSize()));
	safeZoneRectangle.setFillColor(sf::Color(0, 0, 255, 50));

	safeZoneRectangle.setPosition(windowSize.x/8, windowSize.y/2.5);

	safeZoneText.setString("You are safe when touching this.");
	safeZoneText.setCharacterSize(20);
	safeZoneText.setPosition(safeZoneRectangle.getPosition().x + safeZoneRectangle.getGlobalBounds().width + 10, safeZoneRectangle.getPosition().y);
	safeZoneText.setFillColor(sf::Color::Black);

	finishZoneRectangle.setSize(sf::Vector2f(blockGrid.getBlockSize(), blockGrid.getBlockSize()));
	finishZoneRectangle.setFillColor(sf::Color(255, 0, 0, 50));

	finishZoneRectangle.setPosition(windowSize.x/8, windowSize.y/2);

	finishZoneText.setString("Touch this to win");
	finishZoneText.setCharacterSize(20);
	finishZoneText.setPosition(finishZoneRectangle.getPosition().x + finishZoneRectangle.getGlobalBounds().width + 10, finishZoneRectangle.getPosition().y);
	finishZoneText.setFillColor(sf::Color::Black);
}

Screen * InstructionsScreen::update(sf::RenderWindow & window)
{
	sf::Event evt;

	while (window.pollEvent(evt))
	{
		if (evt.type == sf::Event::Closed)
			return nullptr;

		if (evt.type == sf::Event::MouseButtonPressed)
		{
			if (evt.mouseButton.x >= backText.getGlobalBounds().left && evt.mouseButton.y >= backText.getGlobalBounds().top &&
				evt.mouseButton.x < backText.getGlobalBounds().left + backText.getGlobalBounds().width &&
				evt.mouseButton.y < backText.getGlobalBounds().top + backText.getGlobalBounds().height)
			{
				return new MainMenuScreen(sf::Vector2i(window.getSize().x, window.getSize().y), *font);
			}
		}
	}

	return this;
}

void InstructionsScreen::draw(sf::RenderTarget & target)
{
	target.clear(sf::Color::White);

	target.draw(titleText);
	target.draw(backText);
	target.draw(playerText);
	target.draw(bulletText);
	target.draw(turretText);
	target.draw(movingTurretText);
	target.draw(safeZoneText);
	target.draw(finishZoneText);

	target.draw(playerRectangle);
	target.draw(bulletRectangle);
	target.draw(turretRectangle);
	target.draw(movingTurretRectangle);
	target.draw(safeZoneRectangle);
	target.draw(finishZoneRectangle);
}

DeadScreen::DeadScreen(sf::Vector2i windowSize, sf::Font & font)
{
	this->font = &font;
	this->windowSize = windowSize;

	diedText.setFont(font);
	explanationText.setFont(font);
	backText.setFont(font);

	diedText.setFillColor(sf::Color::Black);
	diedText.setCharacterSize(30);
	diedText.setString("You died!");
	diedText.setOrigin(diedText.getLocalBounds().width/2, diedText.getLocalBounds().height/2);
	diedText.setPosition(windowSize.x/2, windowSize.y/5);

	explanationText.setFillColor(sf::Color::Black);
	explanationText.setCharacterSize(20);
	explanationText.setString("Looks like you just aren't good enough.");
	explanationText.setOrigin(explanationText.getLocalBounds().width/2, explanationText.getLocalBounds().height/2);
	explanationText.setPosition(windowSize.x/2, windowSize.y/3);

	backText.setFillColor(sf::Color::Black);
	backText.setCharacterSize(20);
	backText.setString("Main Menu");
	backText.setOrigin(backText.getLocalBounds().width/2, backText.getLocalBounds().height/2);
	backText.setPosition(windowSize.x/2, windowSize.y/2);
		
}

Screen * DeadScreen::update(sf::RenderWindow & window)
{
	sf::Event evt;

	while (window.pollEvent(evt))
	{
		if (evt.type == sf::Event::Closed)
			return nullptr;

		if (evt.type == sf::Event::MouseButtonPressed)
		{
			if (evt.mouseButton.x >= backText.getGlobalBounds().left && evt.mouseButton.y >= backText.getGlobalBounds().top &&
				evt.mouseButton.x < backText.getGlobalBounds().left + backText.getGlobalBounds().width &&
				evt.mouseButton.y < backText.getGlobalBounds().top + backText.getGlobalBounds().height)
			{
				return new MainMenuScreen(windowSize, *font);
			}
		}
	}

	return this;
}

void DeadScreen::draw(sf::RenderTarget & target)
{
	target.clear(sf::Color::White);
	target.draw(diedText);
	target.draw(explanationText);
	target.draw(backText);
}


WinScreen::WinScreen(sf::Vector2i windowSize, sf::Font & font)
{
	this->font = &font;
	this->windowSize = windowSize;

	winText.setFont(font);
	explanationText.setFont(font);
	backText.setFont(font);

	winText.setFillColor(sf::Color::Black);
	winText.setCharacterSize(30);
	winText.setString("You win!");
	winText.setOrigin(winText.getLocalBounds().width/2, winText.getLocalBounds().height/2);
	winText.setPosition(windowSize.x/2, windowSize.y/5);

	explanationText.setFillColor(sf::Color::Black);
	explanationText.setCharacterSize(20);
	explanationText.setString("Get over it.");
	explanationText.setOrigin(explanationText.getLocalBounds().width/2, explanationText.getLocalBounds().height/2);
	explanationText.setPosition(windowSize.x/2, windowSize.y/3);

	backText.setFillColor(sf::Color::Black);
	backText.setCharacterSize(20);
	backText.setString("Main Menu");
	backText.setOrigin(backText.getLocalBounds().width/2, backText.getLocalBounds().height/2);
	backText.setPosition(windowSize.x/2, windowSize.y/2);
}

Screen * WinScreen::update(sf::RenderWindow & window)
{
	sf::Event evt;

	while (window.pollEvent(evt))
	{
		if (evt.type == sf::Event::Closed)
			return nullptr;

		if (evt.type == sf::Event::MouseButtonPressed)
		{
			if (evt.mouseButton.x >= backText.getGlobalBounds().left && evt.mouseButton.y >= backText.getGlobalBounds().top &&
				evt.mouseButton.x < backText.getGlobalBounds().left + backText.getGlobalBounds().width &&
				evt.mouseButton.y < backText.getGlobalBounds().top + backText.getGlobalBounds().height)
			{
				return new MainMenuScreen(windowSize, *font);
			}
		}
	}

	return this;
}

void WinScreen::draw(sf::RenderTarget & target)
{
	target.clear(sf::Color::White);
	target.draw(winText);
	target.draw(explanationText);
	target.draw(backText);
}




int main()
{
	sf::Font font;

	font.loadFromFile("arial.ttf");

	sf::RenderWindow window(sf::VideoMode(700, 700), "Gun Game"); 

	Screen * currentScreen = new MainMenuScreen(sf::Vector2i(window.getSize().x, window.getSize().y), font);

	sf::Clock clock;


	while (true)
	{
		if (clock.getElapsedTime().asSeconds() > 0.002)
		{
			clock.restart();

			//UPDATES

			Screen * screen = currentScreen->update(window);

			if (screen == nullptr)
			{
				delete currentScreen;

				std::exit(0);
			}

			if (screen != currentScreen)
			{
				delete currentScreen;

				currentScreen = screen;

				continue;
			}

			//DRAW

			currentScreen->draw(window);

			window.display();
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}