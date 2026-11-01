#include "harness.h"
#define BOTS_MAXIMUM 63
enum { _bots_goal_none, _bots_goal_item };
struct bots_bot { long unit_index, powerup_index; short goal_kind; real powerup_distance; };
static struct { struct bots_bot bots[BOTS_MAXIMUM]; } bots_globals;
#include "under_test.inc"
int main(int argc, char **argv)
{
    struct bots_bot *bot = &bots_globals.bots[1], *claim = &bots_globals.bots[0];
    CHECK(argc == 2, "case");
    claim->unit_index = 10; claim->powerup_index = 42; claim->goal_kind = _bots_goal_item;
    claim->powerup_distance = 5;
    if (!strcmp(argv[1], "closer"))
    {
        CHECK(bots_powerup_claimed(bot, 42, 10), "farther bot stole claim");
        CHECK(!bots_powerup_claimed(bot, 42, 3), "nearer bot refused claim");
    }
    else if (!strcmp(argv[1], "tie"))
    {
        CHECK(bots_powerup_claimed(bot, 42, 5), "unstable tie");
        CHECK(!bots_powerup_claimed(claim, 42, 5), "claimant blocks itself");
    }
    else if (!strcmp(argv[1], "released"))
    {
        claim->unit_index = NONE;
        CHECK(!bots_powerup_claimed(bot, 42, 10), "dead bot retained claim");
        claim->unit_index = 10; claim->goal_kind = _bots_goal_none;
        CHECK(!bots_powerup_claimed(bot, 42, 10), "changed goal retained claim");
    }
    else CHECK(0, "case");
    return 0;
}
