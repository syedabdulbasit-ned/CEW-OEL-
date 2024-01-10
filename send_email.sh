#!/bin/bash

recipient_email="abdulbasit.shaz@gmail.com"  # Update with your recipient's email
subject="ALERT: Something Went Wrong"
body="Detailed description of the issue."

# Check if uuencode is installed
if ! command -v uuencode &> /dev/null; then
    echo "Error: uuencode is not installed. Please install the 'sharutils' package."
    exit 1
fi

# Construct the email headers and body
email_data=$(cat <<EOF
From: basit4502929@cloud.neduet.edu.pk
To: $recipient_email
Subject: $subject

$body
EOF
)

# Save the email data to a temporary file
email_file=$(mktemp)
echo -e "$email_data" > "$email_file"

# Send email with attachment
(uuencode anamoly.log anamoly.log; cat "$email_file") | mail -s "$subject" "$recipient_email"

if [ $? -eq 0 ]; then
    echo "Email sent successfully."
else
    echo "Error: Failed to send email."
fi

# Remove the temporary file
rm -f "$email_file"
