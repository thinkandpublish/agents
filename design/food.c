#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>  // For stat() to check files
#include <errno.h>     // For errno

// Configuration (adjust these)
#define REPO_DIR "./agents"  // Local path to your cloned agents.git repo
#define LOAD_THRESHOLD_HIGH 1.00
#define LOAD_THRESHOLD_LOW 0.80
#define SLEEP_SECONDS 5  // Yield time if overloaded
#define LOOP_DELAY_SECONDS 10  // Main loop delay
#define MAX_COMMAND_LEN 512  // Buffer size for system commands

// Function to get load average (improved: uses /proc/loadavg for Linux; fallback to uptime)
double get_load_average() {
    FILE *fp;
    double load = -1.0;
    
    // Try /proc/loadavg first (faster, more reliable on Linux)
    if ((fp = fopen("/proc/loadavg", "r")) != NULL) {
        if (fscanf(fp, "%lf", &load) != 1) {
            load = -1.0;  // Failed to parse
        }
        fclose(fp);
    } else {
        // Fallback to uptime (cross-platform but slower)
        if ((fp = popen("uptime", "r")) != NULL) {
            char buffer[256];
            if (fgets(buffer, sizeof(buffer), fp) != NULL) {
                char *load_str = strstr(buffer, "load average:");
                if (load_str && sscanf(load_str + 14, "%lf", &load) != 1) {
                    load = -1.0;  // Parsing failed
                }
            }
            pclose(fp);
        }
    }
    
    if (load < 0.0) {
        fprintf(stderr, "Error: Could not retrieve load average (errno: %d)\n", errno);
    }
    return load;
}

// Helper: Check if a file exists
int file_exists(const char *filename) {
    struct stat buffer;
    return (stat(filename, &buffer) == 0);
}

// Helper: Safely execute a system command and check success
int safe_system(const char *command) {
    int ret = system(command);
    if (ret == -1) {
        fprintf(stderr, "Error: Failed to execute command '%s' (errno: %d)\n", command, errno);
        return -1;
    }
    return WEXITSTATUS(ret);  // Return exit code
}

// Study Agent: Sense and monitor (improved: better error handling, no hardcoded system call)
int study_agent() {
    printf("Study: Sensing environment...\n");
    char command[MAX_COMMAND_LEN];
    snprintf(command, sizeof(command), "ls -la %s > directory_listing.txt", REPO_DIR);
    
    if (safe_system(command) != 0) {
        fprintf(stderr, "Study: Failed to list directory\n");
        return -1;
    }
    
    // Optional: Check for changes (e.g., compare to previous listing)
    // For now, assume it always succeeds and triggers analysis
    return 0;
}

// Analyze Agent: Solve/evaluate (improved: checks file existence, parses response more robustly)
int analyze_agent() {
    printf("Analyze: Evaluating for corrections...\n");
    double load = get_load_average();
    if (load < 0.0) {
        fprintf(stderr, "Analyze: Load check failed\n");
        return 0;  // Default to no action
    }
    if (load > LOAD_THRESHOLD_HIGH) {
        printf("Anomaly: Load too high (%.2f). Yielding.\n", load);
        return 0;  // No action, yield
    }
    
    // Feed to Grok (mock: replace with real API call)
    // Real integration: Use libcurl to POST to xAI's Grok API (e.g., https://api.x.ai/v1/chat/completions)
    // Example: curl -X POST -H "Authorization: Bearer YOUR_KEY" -d '{"prompt": "Analyze..."}' API_URL > grok_analysis.txt
    char feed_command[MAX_COMMAND_LEN];
    snprintf(feed_command, sizeof(feed_command), 
             "echo 'Mock Grok: Analyzing directory_listing.txt shows potential for new artifact on AI agents.' > grok_analysis.txt");
    // TODO: Replace with: curl -s -X POST ... > grok_analysis.txt
    if (safe_system(feed_command) != 0) {
        fprintf(stderr, "Analyze: Failed to query AI\n");
        return 0;
    }
    
    // Parse response: Look for "reprogram" trigger (case-insensitive)
    if (!file_exists("grok_analysis.txt")) {
        fprintf(stderr, "Analyze: grok_analysis.txt not found\n");
        return 0;
    }
    
    FILE *fp = fopen("grok_analysis.txt", "r");
    if (!fp) {
        fprintf(stderr, "Analyze: Could not open grok_analysis.txt\n");
        return 0;
    }
    
    char line[256];
    int trigger_reprogram = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strcasestr(line, "reprogram")) {  // Case-insensitive search
            trigger_reprogram = 1;
            break;
        }
    }
    fclose(fp);
    
    if (trigger_reprogram) {
        printf("Analyze: Triggering reprogram based on AI response.\n");
        return 1;  // Trigger reprogram
    }
    return 0;
}

// Reprogram Agent: Go/execute (improved: fixed filename concatenation, better error handling)
int reprogram_agent() {
    printf("Reprogram: Applying changes...\n");
    
    // Generate new artifact (mock: replace with real API call like in analyze)
    char generate_command[MAX_COMMAND_LEN];
    snprintf(generate_command, sizeof(generate_command), 
             "echo 'Mock new artifact: #include <stdio.h> int main() { printf(\"Hello from agent!\\n\"); return 0; }' > new_artifact.txt");
    // TODO: Replace with curl to generate code based on grok_analysis.txt
    if (safe_system(generate_command) != 0) {
        fprintf(stderr, "Reprogram: Failed to generate artifact\n");
        return -1;
    }
    
    // Publish to GitHub: Copy with timestamped name, add/commit/push
    time_t now = time(NULL);
    char artifact_filename[256];
    snprintf(artifact_filename, sizeof(artifact_filename), "%s/artifact_%ld.txt", REPO_DIR, now);
    
    char cp_command[MAX_COMMAND_LEN];
    snprintf(cp_command, sizeof(cp_command), "cp new_artifact.txt %s", artifact_filename);
    if (safe_system(cp_command) != 0) {
        fprintf(stderr, "Reprogram: Failed to copy artifact\n");
        return -1;
    }
    
    // Git commands (assumes repo is initialized and credentials set)
    char git_commands[MAX_COMMAND_LEN * 3];  // Buffer for chained commands
    snprintf(git_commands, sizeof(git_commands), 
             "cd %s && git add . && git commit -m 'Agent-generated artifact' && git push", REPO_DIR);
    if (safe_system(git_commands) != 0) {
        fprintf(stderr, "Reprogram: Failed to publish to GitHub\n");
        return -1;
    }
    
    printf("Reprogram: Artifact published to GitHub as %s.\n", artifact_filename);
    return 0;
}

// Main loop (Meta-Agent overseer) (improved: better load checks, error propagation)
int main() {
    printf("Starting Agent Loop...\n");
    while (1) {
        double load = get_load_average();
        if (load < 0.0) {
            fprintf(stderr, "Main: Load check failed, skipping cycle\n");
            sleep(LOOP_DELAY_SECONDS);
            continue;
        }
        if (load > LOAD_THRESHOLD_HIGH) {
            printf("Overloaded (%.2f). Sleeping...\n", load);
            sleep(SLEEP_SECONDS);
            continue;
        }
        
        // Study (sense)
        if (study_agent() != 0) {
            sleep(LOOP_DELAY_SECONDS);
            continue;
        }
        
        // Analyze and potentially reprogram
        if (analyze_agent()) {
            if (reprogram_agent() != 0) {
                fprintf(stderr, "Main: Reprogram failed, continuing loop\n");
            }
        }
        
        sleep(LOOP_DELAY_SECONDS);
    }
    return 0;
}
