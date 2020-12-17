void kernel next_Gen(global int* grid, global int* gridTemp, int width, int height, int useTemp)
{
    int x = get_global_id(0);
    int y = get_global_id(1);
    int id = width * y + x;

    int yDown = y - 1;
    if (yDown < 0) {
        yDown += height;
    }
    int xLeft = x - 1;
    if (xLeft < 0) {
        xLeft += width;
    }
    int yUp = y + 1;
    if (yUp >= height) {
        yUp -= height;
    }
    int xRight = x + 1;
    if (xRight >= width) {
        xRight -= width;
    }

    if (useTemp > 0)
    {
        int neighboursAlive;
        neighboursAlive = gridTemp[xLeft + width * yDown] + gridTemp[x + width * yDown]
                        + gridTemp[xRight + width * yDown] + gridTemp[xLeft + width * y]
                        + gridTemp[xRight + width * y] + gridTemp[xLeft + width * yUp]
                        + gridTemp[x + width * yUp] + gridTemp[xRight + width * yUp];

        
        int cell = gridTemp[id];
        // Here we have explicitly all of the game rules
        if (cell == 1 && neighboursAlive < 2)
            grid[id] = 0;
        else if (cell == 1 && (neighboursAlive == 2 || neighboursAlive == 3))
            grid[id] = 1;
        else if (cell == 1 && neighboursAlive > 3)
            grid[id] = 0;
        else if (cell == 0 && neighboursAlive == 3)
            grid[id] = 1;
        else
            grid[id] = cell;
    }
    else
    {
        int neighboursAlive;
        neighboursAlive = grid[xLeft + width * yDown] + grid[x + width * yDown]
                        + grid[xRight + width * yDown] + grid[xLeft + width * y]
                        + grid[xRight + width * y] + grid[xLeft + width * yUp]
                        + grid[x + width * yUp] + grid[xRight + width * yUp];

        int cell = grid[id];
        // Here we have explicitly all of the game rules
        if (cell == 1 && neighboursAlive < 2)
            gridTemp[id] = 0;
        else if (cell == 1 && (neighboursAlive == 2 || neighboursAlive == 3))
            gridTemp[id] = 1;
        else if (cell == 1 && neighboursAlive > 3)
            gridTemp[id] = 0;
        else if (cell == 0 && neighboursAlive == 3)
            gridTemp[id] = 1;
        else
            gridTemp[id] = cell;
    }

}
